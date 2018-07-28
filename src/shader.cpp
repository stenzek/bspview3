#include "pch.h"
#include "shader.h"
#include "util.h"
#include "vertex_array.h"
#include <cassert>
#include <cstdio>

static GLuint s_last_program = 0;

Shader::Shader(GLenum type, GLuint id) : m_type(type), m_id(id) {}

Shader::~Shader()
{
  glDeleteShader(m_id);
}

std::unique_ptr<Shader> Shader::Create(GLenum type, const char* source, size_t length)
{
  GLuint id = glCreateShader(type);
  GLint lengthi = static_cast<GLint>(length);
  glShaderSource(id, 1, &source, &lengthi);
  glCompileShader(id);

  GLint status = GL_FALSE;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  GLint info_log_size = 0;
  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_size);

  if (status != GL_TRUE || info_log_size > 0)
  {
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(info_log_size + 1);
    glGetShaderInfoLog(id, info_log_size, nullptr, buf.get());
    buf[info_log_size] = 0;

    std::fprintf(stderr, "Compile shader %s:\n%s\n", (status == GL_TRUE) ? "succeeded with warnings" : "failed",
                 buf.get());
    if (status != GL_TRUE)
    {
      glDeleteShader(id);
      return nullptr;
    }
  }

  return std::unique_ptr<Shader>(new Shader(type, id));
}

ShaderProgram::ShaderProgram(GLuint program_id, std::vector<GLint> uniform_locations)
  : m_id(program_id), m_uniform_locations(std::move(uniform_locations))
{
}

ShaderProgram::~ShaderProgram()
{
  if (s_last_program == m_id)
  {
    glUseProgram(0);
    s_last_program = 0;
  }

  glDeleteProgram(m_id);
}

void ShaderProgram::Bind()
{
  if (s_last_program == m_id)
    return;

  glUseProgram(m_id);
  s_last_program = m_id;
}

void ShaderProgram::SetUniform(size_t index, const float val)
{
  if (m_uniform_locations[index] >= 0)
    glUniform1f(index, val);
}

void ShaderProgram::SetUniform(size_t index, const glm::vec2& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniform2fv(index, 1, glm::value_ptr(val));
}

void ShaderProgram::SetUniform(size_t index, const glm::vec3& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniform3fv(index, 1, glm::value_ptr(val));
}

void ShaderProgram::SetUniform(size_t index, const glm::vec4& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniform4fv(index, 1, glm::value_ptr(val));
}

void ShaderProgram::SetUniform(size_t index, const glm::mat2& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniformMatrix2fv(index, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::SetUniform(size_t index, const glm::mat3& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniformMatrix3fv(index, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::SetUniform(size_t index, const glm::mat4& val)
{
  if (m_uniform_locations[index] >= 0)
    glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(val));
}

std::unique_ptr<ShaderProgram> ShaderProgram::Create(const VertexAttribute* attributes, size_t num_attributes,
                                                     const Shader* vertex_shader, const Shader* fragment_shader,
                                                     size_t num_samplers /*= 0*/, size_t num_fs_outputs /*= 1*/,
                                                     const char** uniform_names /*= nullptr*/,
                                                     size_t num_uniform_names /*= 0*/)
{
  assert(vertex_shader->GetType() == GL_VERTEX_SHADER && fragment_shader->GetType() == GL_FRAGMENT_SHADER);

  GLuint id = glCreateProgram();
  glAttachShader(id, vertex_shader->GetGLID());
  glAttachShader(id, fragment_shader->GetGLID());

  for (size_t i = 0; i < num_attributes; i++)
    glBindAttribLocation(id, static_cast<GLuint>(i), attributes[i].shader_name.c_str());

  for (size_t i = 0; i < num_fs_outputs; i++)
  {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "ocol%zu", i);
    glBindFragDataLocation(id, 0, buf);
  }

  glLinkProgram(id);

  GLint status = 0;
  glGetProgramiv(id, GL_LINK_STATUS, &status);
  GLint info_log_size = 0;
  glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_size);
  if (status == GL_FALSE || info_log_size > 0)
  {
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(info_log_size + 1);
    glGetProgramInfoLog(id, info_log_size, nullptr, buf.get());
    buf[info_log_size] = 0;

    std::fprintf(stderr, "Link program %s:\n%s\n", (status == GL_TRUE) ? "succeeded with warnings" : "failed",
                 buf.get());
    if (status != GL_TRUE)
    {
      glDeleteShader(id);
      return nullptr;
    }
  }

  glUseProgram(id);

  // Bind samples to texture units at create time. Saves doing it later.
  for (size_t i = 0; i < num_samplers; i++)
  {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "samp%zu", i);
    GLint loc = glGetUniformLocation(id, buf);
    if (loc >= 0)
      glUniform1i(loc, static_cast<GLint>(i));
  }

  std::vector<GLint> uniform_locations;
  for (size_t i = 0; i < num_uniform_names; i++)
    uniform_locations.push_back(glGetUniformLocation(id, uniform_names[i]));

  glUseProgram(s_last_program);

  return std::unique_ptr<ShaderProgram>(new ShaderProgram(id, std::move(uniform_locations)));
}
