#include "pch.h"
#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "util.h"
#include <algorithm>
#include <cassert>
#include <stb_image.h>

static GLuint s_texture_bindings[Texture::NUM_TEXTURE_UNITS] = {};
static size_t s_active_texture = 0;

static u32 GetNumMipmaps(u32 width, u32 height)
{
  u32 num_levels = 1;
  while (width > 1 || height > 1)
  {
    width = std::max(width / 2, u32(1));
    height = std::max(height / 2, u32(1));
    num_levels++;
  }

  return num_levels;
}

static u32 GetMipSize(u32 size, u32 mip_level)
{
  return std::max(size >> mip_level, u32(1));
}

static void SetActiveTexture(size_t i)
{
  if (s_active_texture != i)
  {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
    s_active_texture = i;
  }
}

static GLenum GetGLInternalFormat(Texture::Format format)
{
  switch (format)
  {
    case Texture::Format::FORMAT_R8:
      return GL_R8;
    case Texture::Format::FORMAT_RGB8:
      return GL_RGB8;
    case Texture::Format::FORMAT_RGBA8:
      return GL_RGBA8;
    default:
      return GL_RGBA;
  }
}

static GLenum GetGLFormat(Texture::Format format)
{
  switch (format)
  {
    case Texture::Format::FORMAT_R8:
      return GL_RED;
    case Texture::Format::FORMAT_RGB8:
      return GL_RGB;
    case Texture::Format::FORMAT_RGBA8:
      return GL_RGBA;
    default:
      return GL_RGBA;
  }
}

static GLenum GetGLType(Texture::Format format)
{
  switch (format)
  {
    case Texture::Format::FORMAT_R8:
      return GL_UNSIGNED_BYTE;
    case Texture::Format::FORMAT_RGB8:
    case Texture::Format::FORMAT_RGBA8:
      return GL_UNSIGNED_BYTE;
    default:
      return GL_UNSIGNED_BYTE;
  }
}

static GLenum GetMipDataSize(Texture::Format format, u32 width, u32 height)
{
  switch (format)
  {
    case Texture::Format::FORMAT_R8:
      return width * height;
    case Texture::Format::FORMAT_RGB8:
      return width * height * 3;
    case Texture::Format::FORMAT_RGBA8:
      return width * height * 4;
    default:
      return 0;
  }
}

Texture::Texture(GLuint id, Format format, u32 width, u32 height, u32 levels)
  : m_id(id), m_format(format), m_width(width), m_height(height), m_levels(levels)
{
}

Texture::~Texture()
{
  for (size_t i = 0; i < NUM_TEXTURE_UNITS; i++)
  {
    if (s_texture_bindings[i] == m_id)
    {
      SetActiveTexture(i);
      glBindTexture(GL_TEXTURE_2D, 0);
      s_texture_bindings[i] = 0;
    }
  }
}

void Texture::Bind(size_t texture_unit) const
{
  assert(texture_unit < NUM_TEXTURE_UNITS);
  if (s_texture_bindings[texture_unit] == m_id)
    return;

  SetActiveTexture(texture_unit);
  glBindTexture(GL_TEXTURE_2D, m_id);
  s_texture_bindings[texture_unit] = m_id;
}

std::unique_ptr<Texture> Texture::Create(Format format, u32 width, u32 height, s32 levels, const void* data,
                                         bool linear_filtering /*= true*/, bool wrap_u /*= true*/,
                                         bool wrap_v /*= true*/)
{
  SetActiveTexture(MUTABLE_TEXTURE_UNIT);

  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  u32 mip_levels = (levels < 0) ? GetNumMipmaps(width, height) : static_cast<u32>(levels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip_levels - 1);

  const char* data_ptr = reinterpret_cast<const char*>(data);
  const GLenum gl_internal_format = GetGLInternalFormat(format);
  const GLenum gl_format = GetGLFormat(format);
  const GLenum gl_type = GetGLType(format);

  // Always has the first mip level.
  glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, gl_type, data_ptr);
  data_ptr += GetMipDataSize(format, width, height);

  if (levels > 0)
  {
    // Load other mip levels.
    for (s32 i = 1; i < levels; i++)
    {
      const u32 mip_width = GetMipSize(width, i);
      const u32 mip_height = GetMipSize(height, i);
      glTexImage2D(GL_TEXTURE_2D, i, gl_internal_format, mip_width, mip_height, 0, gl_format, gl_type, data_ptr);
      data_ptr += GetMipDataSize(format, mip_width, mip_height);
    }
  }
  else
  {
    // Auto mipmap generation.
    glEnable(GL_TEXTURE_2D);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_u ? GL_REPEAT : GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_v ? GL_REPEAT : GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear_filtering ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  linear_filtering ? (mip_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : GL_NEAREST);

  return std::unique_ptr<Texture>(new Texture(id, format, width, height, mip_levels));
}

std::unique_ptr<Texture> Texture::LoadFromFile(const char* filename, bool generate_mipmaps /*= true*/,
                                               bool linear_filtering /*= true*/, bool wrap_u /*= true*/,
                                               bool wrap_v /*= true*/)
{
  auto fp = Util::FOpenUniquePtr(filename, "rb");
  if (!fp)
  {
    std::fprintf(stderr, "Failed to open texture: '%s'\n", filename);
    return 0;
  }

  int width, height, components;
  std::unique_ptr<stbi_uc[], void (*)(stbi_uc*)> data(stbi_load_from_file(fp.get(), &width, &height, &components, 0),
                                                      [](stbi_uc* ptr) { STBI_FREE(ptr); });
  if (!data)
  {
    std::fprintf(stderr, "Failed to load texture: '%s': %s\n", filename,
                 stbi_failure_reason() ? stbi_failure_reason() : "unknown error");
    return 0;
  }

  if (width <= 0 || height <= 0 || components < 1 || components > 4)
  {
    std::fprintf(stdout, "Failed to load texture '%s': Invalid dimensions (%dx%dx%d)\n", filename, width, height,
                 components);
    return 0;
  }

  return Create(components == 3 ? Format::FORMAT_RGB8 : Format::FORMAT_RGBA8, width, height, generate_mipmaps ? -1 : 1,
                data.get(), linear_filtering, wrap_u, wrap_v);
}

void Texture::Unbind(size_t texture_unit)
{
  if (s_texture_bindings[texture_unit] == 0)
    return;

  SetActiveTexture(texture_unit);
  glBindTexture(GL_TEXTURE_2D, 0);
  s_texture_bindings[texture_unit] = 0;
}

std::unique_ptr<Texture> Texture::CreateSingleColorTexture(u32 color, u32 width /*= 1*/, u32 height /*= 1*/)
{
  std::vector<u32> tex_data(width * height);
  std::fill_n(tex_data.begin(), width * height, color);

  return Create(Format::FORMAT_RGBA8, width, height, -1, tex_data.data(), true, true, true);
}

std::unique_ptr<Texture> Texture::CreateCheckerboardTexture(u32 checkerboard_color_1 /* = 0xffcc483f */,
                                                            u32 checkerboard_color_2 /* = 0xffffffff */,
                                                            u32 checkerboard_size /* = 8 */, u32 width /* = 64 */,
                                                            u32 height /* = 64 */)
{
  std::vector<u32> tex_data(width * height);
  u32* out_tex_data = tex_data.data();
  const u32 colors[2] = {checkerboard_color_1, checkerboard_color_2};
  u32 row_start_color = 0;
  for (u32 y = 0; y < height; y++)
  {
    if (y > 0 && ((y % checkerboard_size) == 0))
      row_start_color ^= 1;

    u32 x = 0;
    u32 current_color = row_start_color;
    while (x < width)
    {
      for (u32 offset = 0; x < width && offset < checkerboard_size; x++, offset++)
        *(out_tex_data++) = colors[current_color];

      current_color ^= 1;
    }
  }

  return Create(Format::FORMAT_RGBA8, width, height, -1, tex_data.data(), true, true, true);
}
