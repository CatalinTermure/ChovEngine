#include "rendering/opengl/shader_allocator.h"

#include <fstream>

#include <absl/log/log.h>

namespace chove::rendering::opengl {

namespace {
std::string ReadFile(const std::filesystem::path &path) {
  std::ifstream inf(path);
  if (!inf) {
    throw std::runtime_error("Failed to open file " + path.string());
  }
  inf.seekg(0, std::ios::end);
  std::string result;
  result.resize(inf.tellg());
  inf.seekg(0, std::ios::beg);
  inf.read(result.data(), static_cast<std::streamsize>(result.size()));
  return result;
}

void LogShaderCompileIssues(GLuint shader) {
  GLint success;
  GLchar log[512];

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, log);
    LOG(ERROR) << "Shader compilation error: " << log;
  }
}

void LogShaderLinkIssues(GLuint program) {
  GLint success;
  GLchar log[512];

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(program, 512, nullptr, log);
    LOG(ERROR) << "Shader linking error: " << log;
  }
}

std::vector<std::string> GetDefinesForFlags(ShaderFlags flags) {
  std::vector<std::string> result;
  if (static_cast<uint64_t>(flags) & static_cast<uint64_t>(ShaderFlags::kNoDiffuseTexture)) {
    result.emplace_back("#define NO_DIFFUSE_TEXTURE\n");
  }
  return result;
}

constexpr const char *kShaderVersion = "#version 420 core\n";

}

ShaderAllocator::~ShaderAllocator() {
  std::vector<GLuint> shaders;
  for (auto &[shader, _] : shader_ref_counts) {
    glDeleteProgram(shader);
  }
}

GLuint ShaderAllocator::AllocateShader(const std::filesystem::path &vertex_shader_path,
                                       ShaderFlags vertex_shader_flags,
                                       const std::filesystem::path &fragment_shader_path,
                                       ShaderFlags fragment_shader_flags) {
  ShaderInfo info = {vertex_shader_path, vertex_shader_flags, fragment_shader_path, fragment_shader_flags};

  if (shader_creation_cache_.contains(info) && shader_ref_counts.contains(shader_creation_cache_.at(info))) {
    // second check is needed because the shader might have been deallocated
    shader_ref_counts.at(shader_creation_cache_.at(info)) += 1;
    return shader_creation_cache_.at(info);
  }

  LOG(INFO) << "Reading vertex shader from " << vertex_shader_path;
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  std::vector<const GLchar *> sources;
  std::vector<GLint> lengths;
  std::vector<std::string> defines = GetDefinesForFlags(vertex_shader_flags);
  std::string shader_source = ReadFile(vertex_shader_path);
  sources.push_back(kShaderVersion);
  lengths.push_back(static_cast<GLint>(strlen(kShaderVersion)));
  for (const std::string &define : defines) {
    sources.push_back(define.c_str());
    lengths.push_back(static_cast<GLint>(define.size()));
  }
  sources.push_back(shader_source.c_str());
  lengths.push_back(static_cast<GLint>(shader_source.size()));
  glShaderSource(vertex_shader, static_cast<GLsizei>(sources.size()), sources.data(), lengths.data());

  glCompileShader(vertex_shader);
  LogShaderCompileIssues(vertex_shader);
  LOG(INFO) << "Compiled vertex shader";

  LOG(INFO) << "Reading fragment shader from " << fragment_shader_path;
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  sources.clear();
  lengths.clear();
  defines = GetDefinesForFlags(fragment_shader_flags);
  shader_source = ReadFile(fragment_shader_path);
  sources.push_back(kShaderVersion);
  lengths.push_back(static_cast<GLint>(strlen(kShaderVersion)));
  for (const std::string &define : defines) {
    sources.push_back(define.c_str());
    lengths.push_back(static_cast<GLint>(define.size()));
  }
  sources.push_back(shader_source.c_str());
  lengths.push_back(static_cast<GLint>(shader_source.size()));
  glShaderSource(fragment_shader, static_cast<GLsizei>(sources.size()), sources.data(), lengths.data());

  glCompileShader(fragment_shader);
  LogShaderCompileIssues(fragment_shader);
  LOG(INFO) << "Compiled fragment shader";

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  LogShaderLinkIssues(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  shader_creation_cache_[{vertex_shader_path, vertex_shader_flags, fragment_shader_path, fragment_shader_flags}] =
      program;
  shader_ref_counts[program] = 1;
  return program;
}

void ShaderAllocator::DeallocateShader(GLuint shader) {
  shader_ref_counts.at(shader) -= 1;
  if (shader_ref_counts.at(shader) == 0) {
    shader_ref_counts.erase(shader);
    glDeleteProgram(shader);
  }
}

void ShaderAllocator::InvalidateCache() {
  shader_creation_cache_.clear();
}
}