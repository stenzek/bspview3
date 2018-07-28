#include "pch.h"
#include "buffer.h"
#include "vertex_array.h"
#include <glad.h>

static constexpr size_t BufferTypeIndex(Buffer::Type type)
{
  return static_cast<size_t>(type);
}

static const GLenum s_gl_types[BufferTypeIndex(Buffer::Type::Count)] = {GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
                                                                        GL_UNIFORM_BUFFER};
static GLuint s_last_buffer[BufferTypeIndex(Buffer::Type::Count)];

Buffer::Buffer(Type type, size_t size, u32 id, bool dynamic) : m_type(type), m_size(size), m_id(id), m_dynamic(dynamic)
{
}

Buffer::~Buffer()
{
  const size_t index = BufferTypeIndex(m_type);
  if (s_last_buffer[index] != m_id)
  {
    glDeleteBuffers(1, &m_id);
    return;
  }

  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryUnbind();

  glBindBuffer(s_gl_types[index], 0);
  s_last_buffer[index] = 0;

  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryRebind();

  glDeleteBuffers(1, &m_id);
}

void Buffer::Bind()
{
  const size_t index = BufferTypeIndex(m_type);
  if (s_last_buffer[index] == m_id)
    return;

  glBindBuffer(s_gl_types[index], m_id);
  s_last_buffer[index] = m_id;
}

void Buffer::Unbind()
{
  Unbind(m_type);
}

void Buffer::Unbind(Type type)
{
  const size_t index = BufferTypeIndex(type);
  if (s_last_buffer[index] == 0)
    return;

  glBindBuffer(s_gl_types[index], 0);
  s_last_buffer[index] = 0;
}

void Buffer::Update(size_t offset, size_t count, const void* data)
{
  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryUnbind();

  const size_t index = BufferTypeIndex(m_type);
  if (s_last_buffer[index] != m_id)
  {
    glBindBuffer(s_gl_types[index], m_id);
    s_last_buffer[index] = m_id;
  }

  if (offset == 0 && count == count)
    glBufferData(s_gl_types[index], count, data, m_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
  else
    glBufferSubData(s_gl_types[index], offset, count, data);

  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryRebind();
}

void* Buffer::Map(bool read /* = false */, bool write /* = true */)
{
  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryUnbind();

  const size_t index = BufferTypeIndex(m_type);
  if (s_last_buffer[index] != m_id)
  {
    glBindBuffer(s_gl_types[index], m_id);
    s_last_buffer[index] = m_id;
  }

  GLenum access = 0;
  if (read && !write)
    access = GL_READ_ONLY;
  else if (!read && write)
    access = GL_WRITE_ONLY;
  else
    access = GL_READ_WRITE;

  void* mapped_ptr = glMapBuffer(s_gl_types[index], access);

  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryRebind();

  return mapped_ptr;
}

void Buffer::Unmap()
{
  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryUnbind();

  const size_t index = BufferTypeIndex(m_type);
  if (s_last_buffer[index] != m_id)
  {
    glBindBuffer(s_gl_types[index], m_id);
    s_last_buffer[index] = m_id;
  }

  glUnmapBuffer(s_gl_types[index]);

  if (m_type == Type::VertexBuffer)
    VertexArray::TemporaryRebind();
}

std::unique_ptr<Buffer> Buffer::Create(Type type, size_t size, const void* data, bool dynamic /* = false */)
{
  if (type == Type::VertexBuffer)
    VertexArray::TemporaryUnbind();

  const size_t index = BufferTypeIndex(type);

  GLuint id;
  glGenBuffers(1, &id);

  glBindBuffer(s_gl_types[index], id);

  glBufferData(s_gl_types[index], size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

  glBindBuffer(s_gl_types[index], s_last_buffer[index]);

  if (type == Type::VertexBuffer)
    VertexArray::TemporaryRebind();

  return std::unique_ptr<Buffer>(new Buffer(type, size, id, dynamic));
}
