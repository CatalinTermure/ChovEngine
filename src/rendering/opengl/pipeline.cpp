#include "rendering/opengl/pipeline.h"

namespace chove::rendering::opengl {

Pipeline::Pipeline(Shader vertex_shader, Shader fragment_shader) : vertex_shader(std::move(vertex_shader)),
                                                                   fragment_shader(std::move(fragment_shader)) {

}
}
