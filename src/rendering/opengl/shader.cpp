#include "rendering/opengl/shader.h"

#include <absl/log/log.h>

#include <fstream>

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
    result.emplace_back("#define NO_DIFFUSE_TEXTURE");
  }
  return result;
}

constexpr const char *kShaderVersion = "#version 410 core\n";

}

Shader::Shader(const std::filesystem::path &vertex_shader_path,
               ShaderFlags vertex_shader_flags,
               const std::filesystem::path &fragment_shader_path,
               ShaderFlags fragment_shader_flags) {
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

  program_ = glCreateProgram();
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);
  LogShaderLinkIssues(program_);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

Shader::Shader(Shader &&other) noexcept: program_(other.program_) {
  other.program_ = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  program_ = other.program_;
  other.program_ = 0;
  return *this;
}

Shader::~Shader() {
  if (program_ != 0) {
    glDeleteProgram(program_);
  }
}

void Shader::Use() const {
  glUseProgram(program_);
}
}
