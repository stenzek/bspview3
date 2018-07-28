#pragma once
#include "common.h"
#include <memory>

class Buffer
{
public:
  enum class Type
  {
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    Count
  };

  ~Buffer();

  Type GetType() const { return m_type; }
  size_t GetSize() const { return m_size; }
  u32 GetGLID() const { return m_id; }

  void Bind();
  void Unbind();

  void Update(size_t offset, size_t count, const void* data);

  void* Map(bool read = false, bool write = true);
  void Unmap();

  static std::unique_ptr<Buffer> Create(Type type, size_t size, const void* data, bool dynamic = false);

  static void Unbind(Type type);

private:
  Buffer(Type type, size_t size, u32 id, bool dynamic);

  size_t m_size;
  Type m_type;
  u32 m_id;
  bool m_dynamic;
};