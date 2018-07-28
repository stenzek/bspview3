#pragma once
#include "common.h"
#include <memory>

class Buffer;
class ShaderProgram;
class Texture;
class VertexArray;

class Font
{
public:
  enum : u32
  {
    COLOR_WHITE = 0xFFFFFFFF,
    COLOR_RED = 0xFF0000FF,
    COLOR_GREEN = 0xFF00FF00,
    COLOR_BLUE = 0xFFFF0000
  };

  ~Font();

  void RenderText(s32 x, s32 y, u32 viewport_width, u32 viewport_height, u32 color, const char* text);
  void RenderFormattedText(s32 x, s32 y, u32 viewport_width, u32 viewport_height, u32 color, const char* format, ...);

  static std::unique_ptr<Font> Create();

private:
  static const size_t NUM_CHARS = 256;

  Font(std::unique_ptr<ShaderProgram> program, std::unique_ptr<Texture> texture, std::unique_ptr<Buffer> buffer,
       std::unique_ptr<VertexArray> vertex_array);

  void FillGlyphInfo();

  std::unique_ptr<ShaderProgram> m_program;
  std::unique_ptr<Texture> m_texture;
  std::unique_ptr<Buffer> m_vertex_buffer;
  std::unique_ptr<VertexArray> m_vertex_array;

  struct GlyphInfo
  {
    s32 x0, x1;
    s32 y0, y1;
    s32 xadvance;
    s32 yadvance;
    float u0, v0;
    float u1, v1;
    bool skip;
  };

  GlyphInfo m_glyphs[NUM_CHARS];
};
