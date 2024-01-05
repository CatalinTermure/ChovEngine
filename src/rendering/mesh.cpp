#include "rendering/mesh.h"

#include <absl/log/log.h>
#include <absl/container/flat_hash_map.h>
#include <tiny_obj_loader.h>

namespace chove::rendering {
namespace {
class IndexHash {
 public:
  size_t operator()(const tinyobj::index_t &index) const {
    constexpr absl::Hash<int> hash;
    return hash(index.vertex_index) ^ hash(index.normal_index) ^ hash(index.texcoord_index);
  }
};
class IndexEq {
 public:
  bool operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const {
    return lhs.vertex_index == rhs.vertex_index && lhs.normal_index == rhs.normal_index &&
        lhs.texcoord_index == rhs.texcoord_index;
  }
};
std::optional<std::filesystem::path> GetPath(const std::filesystem::path &path, const std::string &texture_name) {
  if (texture_name.empty()) return std::nullopt;
  return path.parent_path() / texture_name;
}

}

std::vector<Mesh> Mesh::ImportFromObj(const std::filesystem::path &path) {
  tinyobj::ObjReaderConfig reader_config;
  reader_config.mtl_search_path = path.parent_path().string();

  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(path.string(), reader_config)) {
    if (!reader.Error().empty()) {
      LOG(FATAL) << "TinyObjReader error: " << reader.Error();
    }
    LOG(FATAL) << "TinyObjReader error: unknown";
  }

  if (!reader.Warning().empty()) {
    LOG(ERROR) << "TinyObjReader warning: " << reader.Warning();
  }

  if (reader.Valid()) {
    LOG(INFO) << "TinyObjReader: successfully parsed " << path;
  } else {
    LOG(FATAL) << "TinyObjReader: failed to parse " << path;
  }

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &obj_shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t> &obj_materials = reader.GetMaterials();

  std::vector<Material> mesh_materials;
  mesh_materials.reserve(obj_materials.size());
  for (const auto &material : obj_materials) {
    mesh_materials.push_back(Material{
        .shininess = material.shininess,
        .optical_density = material.ior,
        .dissolve = material.dissolve,
        .transmission_filter_color = glm::vec3(material.transmittance[0],
                                               material.transmittance[1],
                                               material.transmittance[2]),
        .ambient_color = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]),
        .diffuse_color = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]),
        .specular_color = glm::vec3(material.specular[0], material.specular[1], material.specular[2]),
        .ambient_texture = GetPath(path, material.ambient_texname),
        .diffuse_texture = GetPath(path, material.diffuse_texname),
        .specular_texture = GetPath(path, material.specular_texname),
        .shininess_texture = GetPath(path, material.specular_highlight_texname),
        .alpha_texture = GetPath(path, material.alpha_texname),
        .bump_texture = GetPath(path, material.bump_texname),
        .displacement_texture = GetPath(path, material.displacement_texname),
        .illumination_model = static_cast<IllumType>(material.illum)
    });

    if (mesh_materials.back().ambient_color == glm::vec3(0.0f, 0.0f, 0.0f)) {
      mesh_materials.back().ambient_color = mesh_materials.back().diffuse_color;
    }
  }

  // One mesh per shape, may merge
  std::vector<Mesh> meshes;
  meshes.reserve(obj_shapes.size());
  for (const auto &shape : obj_shapes) {
    std::vector<glm::vec3> colors;
    std::vector<Vertex> final_vertices;
    std::vector<uint32_t> indices;
    absl::flat_hash_map<tinyobj::index_t, uint32_t, IndexHash, IndexEq> vertex_map;
    for (auto material_id : shape.mesh.material_ids) {
      LOG_IF(ERROR, material_id != shape.mesh.material_ids[0])
              << "Shape has multiple materials, decomposing into multiple meshes is not supported";
    }
    LOG_IF(FATAL, shape.mesh.indices.size() % 3 != 0) << "Shape has non-triangular faces";
    for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
      for (int j = 0; j < 3; ++j) {
        const auto &index = shape.mesh.indices[i + j];
        if (vertex_map.contains(index)) {
          indices.push_back(vertex_map.at(index));
        } else {
          Vertex vertex = {glm::vec3(attrib.vertices[3 * index.vertex_index],
                                     attrib.vertices[3 * index.vertex_index + 1],
                                     attrib.vertices[3 * index.vertex_index + 2]),
                           glm::vec3(attrib.normals[3 * index.normal_index],
                                     attrib.normals[3 * index.normal_index + 1],
                                     attrib.normals[3 * index.normal_index + 2]),
                           index.texcoord_index == -1 ? glm::vec2(0.0f, 0.0f) :
                           glm::vec2(attrib.texcoords[2 * index.texcoord_index],
                                     attrib.texcoords[2 * index.texcoord_index + 1]),
                           glm::vec3(0.0f, 0.0f, 0.0f)};
          vertex_map[index] = final_vertices.size();
          indices.push_back(final_vertices.size());
          final_vertices.push_back(vertex);
          colors.emplace_back(attrib.colors[3 * index.vertex_index],
                              attrib.colors[3 * index.vertex_index + 1],
                              attrib.colors[3 * index.vertex_index + 2]);
        }
      }
      glm::vec3 edge1 = final_vertices[indices[i + 1]].position - final_vertices[indices[i]].position;
      glm::vec3 edge2 = final_vertices[indices[i + 2]].position - final_vertices[indices[i]].position;
      glm::vec2 delta_uv1 = final_vertices[indices[i + 1]].texcoord - final_vertices[indices[i]].texcoord;
      glm::vec2 delta_uv2 = final_vertices[indices[i + 2]].texcoord - final_vertices[indices[i]].texcoord;

      float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
      glm::vec3 tangent = glm::normalize(f * (delta_uv2.y * edge1 - delta_uv1.y * edge2));
      final_vertices[indices[i]].tangent = tangent;
      final_vertices[indices[i + 1]].tangent = tangent;
      final_vertices[indices[i + 2]].tangent = tangent;
    }
    LOG(INFO) << "Shape has " << final_vertices.size() << " vertices and " << indices.size() << " indices";

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::min();
    float max_y = std::numeric_limits<float>::min();
    float max_z = std::numeric_limits<float>::min();
    for (const auto &vertex : final_vertices) {
      min_x = std::min(min_x, vertex.position.x);
      min_y = std::min(min_y, vertex.position.y);
      min_z = std::min(min_z, vertex.position.z);
      max_x = std::max(max_x, vertex.position.x);
      max_y = std::max(max_y, vertex.position.y);
      max_z = std::max(max_z, vertex.position.z);
    }
    BoundingBox bounding_box = {
        .min = glm::vec3(min_x, min_y, min_z),
        .max = glm::vec3(max_x, max_y, max_z)
    };

    meshes.emplace_back(final_vertices, colors, indices, mesh_materials[shape.mesh.material_ids[0]], bounding_box);
  }

  return meshes;
}
} // namespace chove::rendering
