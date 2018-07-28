#pragma once
#include "common.h"
#include <glad.h>
#include <memory>
#include <vector>

class Texture
{
public:
  enum class Format : u32
  {
    FORMAT_R8,
    FORMAT_RGB8,
    FORMAT_RGBA8
  };

  // Use num+1 to not disrupt any of the current bindings.
  static constexpr size_t NUM_TEXTURE_UNITS = 4;
  static constexpr size_t MUTABLE_TEXTURE_UNIT = NUM_TEXTURE_UNITS + 1;

  ~Texture();

  GLuint GetGLID() const { return m_id; }
  Format GetFormat() const { return m_format; }
  u32 GetWidth() const { return m_width; }
  u32 GetHeight() const { return m_height; }
  u32 GetLevels() const { return m_levels; }

  void Bind(size_t texture_unit) const;

  // Set levels to -1 for automatic mipmap generation.
  static std::unique_ptr<Texture> Create(Format format, u32 width, u32 height, s32 levels, const void* data,
                                         bool linear_filtering = true, bool wrap_u = true, bool wrap_v = true);

  static std::unique_ptr<Texture> LoadFromFile(const char* filename, bool generate_mipmaps = true,
                                               bool linear_filtering = true, bool wrap_u = true, bool wrap_v = true);

  static void Unbind(size_t texture_unit);

  static std::unique_ptr<Texture> CreateSingleColorTexture(u32 color, u32 width = 1, u32 height = 1);
  static std::unique_ptr<Texture> CreateCheckerboardTexture(u32 checkerboard_color_1 = 0xffcc483f,
                                                            u32 checkerboard_color_2 = 0xffffffff,
                                                            u32 checkerboard_size = 8, u32 width = 64, u32 height = 64);

private:
  Texture(GLuint id, Format format, u32 width, u32 height, u32 levels);

  GLuint m_id;
  Format m_format;
  u32 m_width;
  u32 m_height;
  u32 m_levels;
};