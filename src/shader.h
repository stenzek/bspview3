#pragma once
#include "common.h"
#include <glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct VertexAttribute;

class Shader
{
public:
  ~Shader();

  const GLenum GetType() const { return m_type; }
  const GLuint GetGLID() const { return m_id; }

  static std::unique_ptr<Shader> Create(GLenum type, const char* source, size_t length);

private:
  Shader(GLenum type, GLuint id);

  GLenum m_type;
  GLuint m_id;
};

class ShaderProgram
{
public:
  ~ShaderProgram();

  GLuint GetGLID() const { return m_id; }

  void Bind();

  void SetUniform(size_t index, const float val);
  void SetUniform(size_t index, const glm::vec2& val);
  void SetUniform(size_t index, const glm::vec3& val);
  void SetUniform(size_t index, const glm::vec4& val);
  void SetUniform(size_t index, const glm::mat2& val);
  void SetUniform(size_t index, const glm::mat3& val);
  void SetUniform(size_t index, const glm::mat4& val);

  static std::unique_ptr<ShaderProgram> Create(const VertexAttribute* attributes, size_t num_attributes,
                                               const Shader* vertex_shader, const Shader* fragment_shader,
                                               size_t num_samplers = 0, size_t num_fs_outputs = 1,
                                               const char** uniform_names = nullptr, size_t num_uniform_names = 0);

private:
  ShaderProgram(GLuint program_id, std::vector<GLint> uniform_locations);

  GLuint m_id;
  std::vector<GLint> m_uniform_locations;
};
