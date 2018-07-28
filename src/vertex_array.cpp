#include "pch.h"
#include "vertex_array.h"
#include "buffer.h"

static GLuint s_last_vao = 0;

VertexArray::VertexArray(const std::vector<VertexAttribute>& attributes, GLuint vao)
  : m_attributes(std::move(attributes)), m_vao(vao)
{
}

VertexArray::~VertexArray()
{
  if (s_last_vao == m_vao)
  {
    glBindVertexArray(0);
    s_last_vao = 0;
  }

  glDeleteVertexArrays(1, &m_vao);
}

void VertexArray::Bind()
{
  if (s_last_vao == m_vao)
    return;

  glBindVertexArray(m_vao);
  s_last_vao = m_vao;
}

void VertexArray::Unbind()
{
  if (s_last_vao == 0)
    return;

  glBindVertexArray(0);
  s_last_vao = 0;
}

void VertexArray::TemporaryUnbind()
{
  if (s_last_vao == 0)
    return;

  glBindVertexArray(0);
}

void VertexArray::TemporaryRebind()
{
  if (s_last_vao == 0)
    return;

  glBindVertexArray(s_last_vao);
}

std::unique_ptr<VertexArray> VertexArray::Create(const VertexAttribute* attributes, size_t num_attributes,
                                                 const Buffer** buffers)
{
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 last_buffer = u32(-1);
  std::vector<VertexAttribute> attributes_vec;

  for (size_t i = 0; i < num_attributes; i++)
  {
    const VertexAttribute& attr = attributes[i];
    attributes_vec.push_back(attr);

    if (last_buffer != attr.buffer)
    {
      glBindBuffer(GL_ARRAY_BUFFER, buffers[attr.buffer] ? buffers[attr.buffer]->GetGLID() : 0);
      last_buffer = attr.buffer;
    }

    glEnableVertexAttribArray(static_cast<GLuint>(i));

    glVertexAttribPointer(static_cast<GLuint>(i), attr.count, attr.type, attr.normalzed ? GL_TRUE : GL_FALSE,
                          attr.stride, reinterpret_cast<void*>(attr.offset));
  }

  glBindVertexArray(s_last_vao);

  return std::unique_ptr<VertexArray>(new VertexArray(std::move(attributes_vec), vao));
}
