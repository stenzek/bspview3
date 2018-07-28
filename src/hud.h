#pragma once
#include "common.h"
#include <glm/glm.hpp>
#include <memory>

class Buffer;
class Camera;
class ShaderProgram;
struct VertexAttribute;
class VertexArray;

class HUD
{
public:
  HUD();
  ~HUD();

  u32 GetViewportWidth() const { return m_viewport_width; }
  u32 GetViewportHeight() const { return m_viewport_height; }

  bool Initialize();
  void Shutdown();

  void SetViewportSize(u32 width, u32 height);

  void Draw3DWireBox(const Camera& camera, const glm::vec3& box_min, const glm::vec3& box_max, u32 color);

  float GetClipSpaceX(float pos) const;
  float GetClipSpaceY(float pos) const;

  bool BeginDraw(u32 primitive_type);

  void AddVertex2D(s32 x, s32 y, float u, float v, u32 color);
  void AddVertex2D(float x, float y, float u, float v, u32 color);
  void AddVertex2DClipSpace(float x, float y, float u, float v, u32 color);
  void AddVertex3D(float x, float y, float z, float u, float v, u32 color);

  void FlushDraw();
  void EndDraw();

  static const VertexAttribute* GetHUDVertexAttributes() { return s_hud_vertex_attributes; }
  static size_t GetHUDVertexAttributeCount() { return ARRAY_SIZE(s_hud_vertex_attributes); }

private:
  static const u32 VERTEX_BUFFER_SIZE = 16384;
  static const u32 MAX_VERTICES_PER_DRAW = 512;

  static const VertexAttribute s_hud_vertex_attributes[3];

#pragma pack(push, 1)
  struct HUDVertex
  {
    float pos[3];
    float tex[2];
    u32 col;
  };
#pragma pack(pop)

  bool CreateBuffers();
  bool CompilePrograms(u32 glsl_version);

  u32 m_viewport_width = 1;
  u32 m_viewport_height = 1;
  float m_rcp_viewport_width = 1.0f;
  float m_rcp_viewport_height = 1.0f;

  std::unique_ptr<ShaderProgram> m_color_program_2D;
  std::unique_ptr<ShaderProgram> m_texture_program_2D;

  std::unique_ptr<ShaderProgram> m_color_program_3D;
  std::unique_ptr<ShaderProgram> m_texture_program_3D;

  std::unique_ptr<Buffer> m_vertex_buffer;
  std::unique_ptr<VertexArray> m_vertex_array;

  char* m_vertex_buffer_mapped_ptr = nullptr;
  std::vector<HUDVertex> m_draw_vertices;
  u32 m_draw_primitive_type = 0;
  u32 m_draw_primitive_count = 0;
};

extern HUD* g_hud;
