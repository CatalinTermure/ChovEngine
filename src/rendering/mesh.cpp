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
    for (auto index : shape.mesh.indices) {
      if (vertex_map.contains(index)) {
        indices.push_back(vertex_map.at(index));
        continue;
      }
      Vertex vertex = {glm::vec3(attrib.vertices[3 * index.vertex_index],
                                 attrib.vertices[3 * index.vertex_index + 1],
                                 attrib.vertices[3 * index.vertex_index + 2]),
                       glm::vec3(attrib.normals[3 * index.normal_index],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]),
                       glm::vec2(attrib.texcoords[2 * index.texcoord_index],
                                 attrib.vertices[2 * index.texcoord_index + 1])};
      vertex_map[index] = final_vertices.size();
      final_vertices.push_back(vertex);
      indices.push_back(vertex_map.at(index));
      colors.emplace_back(attrib.colors[3 * index.vertex_index],
                          attrib.colors[3 * index.vertex_index + 1],
                          attrib.colors[3 * index.vertex_index + 2]);
    }
    LOG(INFO) << "Shape has " << final_vertices.size() << " vertices and " << indices.size() << " indices";

    meshes.emplace_back(final_vertices, colors, indices, mesh_materials[shape.mesh.material_ids[0]]);
  }

  return meshes;
}
} // namespace chove::rendering
