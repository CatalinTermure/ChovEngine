#include "rendering/mesh.h"

#include <absl/log/log.h>
#include <tiny_obj_loader.h>

namespace chove::rendering {

constexpr std::vector<vk::VertexInputAttributeDescription> Mesh::attributes() {
  return {
      vk::VertexInputAttributeDescription{
          0,
          0,
          vk::Format::eR32G32B32A32Sfloat,
          0
      },
      vk::VertexInputAttributeDescription{
          1,
          0,
          vk::Format::eR32G32B32A32Sfloat,
          sizeof(glm::vec4)
      }
  };
}
Mesh Mesh::ImportFromObj(std::filesystem::path path) {
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

  const tinyobj::attrib_t& attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

  return Mesh();
}
}  // namespace chove::rendering
