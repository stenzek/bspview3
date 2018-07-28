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
  ~Font();

  s32 GetTextWidth(const char* text);
  void RenderText(s32 x, s32 y, u32 color, const char* text);
  void RenderFormattedText(s32 x, s32 y, u32 color, const char* format, ...);

  static std::unique_ptr<Font> Create();

private:
  static const size_t NUM_CHARS = 256;

  Font(std::unique_ptr<ShaderProgram> program, std::unique_ptr<Texture> texture);

  void FillGlyphInfo();

  std::unique_ptr<ShaderProgram> m_program;
  std::unique_ptr<Texture> m_texture;

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
