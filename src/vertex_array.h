#pragma once
#include "common.h"
#include <glad.h>
#include <string>
#include <vector>

class Buffer;

struct VertexAttribute
{
  std::string shader_name;
  GLenum type;
  u32 count;
  u32 buffer;
  u32 offset;
  u32 stride;
  bool normalzed;
};

class VertexArray
{
public:
  ~VertexArray();

  const VertexAttribute& GetAttribute(size_t i) const { return m_attributes[i]; }
  const std::vector<VertexAttribute>& GetAttributes() const { return m_attributes; }
  const size_t GetAttributeCount() const { return m_attributes.size(); }

  const GLuint GetGLVAO() const { return m_vao; }

  void Bind();

  static std::unique_ptr<VertexArray> Create(const VertexAttribute* attributes, size_t num_attributes,
                                             const Buffer** buffers);

  static void Unbind();

  static void TemporaryUnbind();
  static void TemporaryRebind();

private:
  VertexArray(const std::vector<VertexAttribute>& attributes, GLuint vao);

  std::vector<VertexAttribute> m_attributes;
  GLuint m_vao;
};
