#include "pch.h"
#include "hud.h"
#include "buffer.h"
#include "camera.h"
#include "shader.h"
#include "statistics.h"
#include "util.h"
#include "vertex_array.h"
#include <cassert>
#include <cstring>

//#define USE_MAPPED_VERTEX_BUFFER

static HUD s_hud;
HUD* g_hud = &s_hud;

const VertexAttribute HUD::s_hud_vertex_attributes[3] = {
  {"in_position", GL_FLOAT, 3, 0, offsetof(HUDVertex, pos), sizeof(HUDVertex), false},
  {"in_tex0", GL_FLOAT, 2, 0, offsetof(HUDVertex, tex), sizeof(HUDVertex), false},
  {"in_color", GL_UNSIGNED_BYTE, 4, 0, offsetof(HUDVertex, col), sizeof(HUDVertex), true}};

static const char s_2d_vertex_shader[] = R"(
#version %d

in vec3 in_position;
in vec2 in_tex0;
in vec4 in_color;

out vec2 v_tex;
out vec4 v_col;

void main()
{
  gl_Position = vec4(in_position, 1.0);
  v_tex = in_tex0;
  v_col = in_color;
}
)";

static const char s_3d_vertex_shader[] = R"(
#version %d

uniform mat4 u_projection;

in vec3 in_position;
in vec2 in_tex0;
in vec4 in_color;

out vec2 v_tex;
out vec4 v_col;

void main()
{
  gl_Position = u_projection * vec4(in_position, 1.0);
  v_tex = in_tex0;
  v_col = in_color;
}
)";

static const char s_color_fragment_shader[] = R"(
#version %d

in vec2 v_tex;
in vec4 v_col;

out vec4 ocol0;

void main()
{
  ocol0 = v_col;
}
)";

static const char s_texture_fragment_shader[] = R"(
#version %d

uniform sampler2D samp0;

in vec2 v_tex;
in vec4 v_col;

out vec4 ocol0;

void main()
{
  ocol0 = texture(samp0, v_tex);
}
)";

HUD::HUD() {}

HUD::~HUD() {}

bool HUD::Initialize()
{
  return CreateBuffers() && CompilePrograms(330);
}

void HUD::Shutdown()
{
  m_color_program_2D.reset();
  m_texture_program_2D.reset();
  m_color_program_3D.reset();
  m_texture_program_3D.reset();
  m_vertex_buffer.reset();
  m_vertex_array.reset();
}

void HUD::SetViewportSize(u32 width, u32 height)
{
  width = std::max(width, u32(1));
  height = std::max(height, u32(1));
  if (m_viewport_width == width && m_viewport_height == height)
    return;

  m_viewport_width = width;
  m_viewport_height = height;
  m_rcp_viewport_width = 1.0f / m_viewport_width;
  m_rcp_viewport_height = 1.0f / m_viewport_height;
}

void HUD::Draw3DWireBox(const Camera& camera, const glm::vec3& box_min, const glm::vec3& box_max, u32 color)
{
  m_color_program_3D->Bind();
  m_color_program_3D->SetUniform(0, camera.GetViewProjectionMatrix());

  BeginDraw(GL_LINES);
  AddVertex3D(box_min.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_min.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_max.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_max.z, 0.0f, 0.0f, color);
  AddVertex3D(box_min.x, box_max.y, box_min.z, 0.0f, 0.0f, color);
  EndDraw();
}

bool HUD::CreateBuffers()
{
  m_vertex_buffer = Buffer::Create(Buffer::Type::VertexBuffer, VERTEX_BUFFER_SIZE, nullptr, true);
  if (!m_vertex_buffer)
    return false;

  const Buffer* va_buffers[] = {m_vertex_buffer.get()};
  m_vertex_array = VertexArray::Create(s_hud_vertex_attributes, ARRAY_SIZE(s_hud_vertex_attributes), va_buffers);
  if (!m_vertex_array)
    return false;

  return true;
}

bool HUD::CompilePrograms(u32 glsl_version)
{
  static const char* three_uniforms[] = {"u_projection"};
  std::string str;

  str = Util::StringFromFormat(s_2d_vertex_shader, glsl_version);
  auto vs_2d = Shader::Create(GL_VERTEX_SHADER, str.c_str(), str.length());
  str = Util::StringFromFormat(s_3d_vertex_shader, glsl_version);
  auto vs_3d = Shader::Create(GL_VERTEX_SHADER, str.c_str(), str.length());
  if (!vs_2d || !vs_3d)
    return false;

  str = Util::StringFromFormat(s_color_fragment_shader, glsl_version);
  auto fs_color = Shader::Create(GL_FRAGMENT_SHADER, str.c_str(), str.length());
  str = Util::StringFromFormat(s_texture_fragment_shader, glsl_version);
  auto fs_tex = Shader::Create(GL_FRAGMENT_SHADER, str.c_str(), str.length());
  if (!fs_color || !fs_tex)
    return false;

  m_color_program_2D = ShaderProgram::Create(s_hud_vertex_attributes, ARRAY_SIZE(s_hud_vertex_attributes), vs_2d.get(),
                                             fs_color.get(), 0, 1, nullptr, 0);
  m_texture_program_2D = ShaderProgram::Create(s_hud_vertex_attributes, ARRAY_SIZE(s_hud_vertex_attributes),
                                               vs_2d.get(), fs_tex.get(), 0, 1, nullptr, 0);
  m_color_program_3D = ShaderProgram::Create(s_hud_vertex_attributes, ARRAY_SIZE(s_hud_vertex_attributes), vs_3d.get(),
                                             fs_color.get(), 0, 1, three_uniforms, ARRAY_SIZE(three_uniforms));
  m_texture_program_3D =
    ShaderProgram::Create(s_hud_vertex_attributes, ARRAY_SIZE(s_hud_vertex_attributes), vs_3d.get(), fs_tex.get(), 0, 1,
                          three_uniforms, ARRAY_SIZE(three_uniforms));
  if (!m_color_program_2D || !m_texture_program_2D || !m_color_program_3D || !m_texture_program_3D)
    return false;

  return true;
}

float HUD::GetClipSpaceX(float pos) const
{
  // 0..width -> 0..1 -> -1..1
  return pos * m_rcp_viewport_width * 2.0f - 1.0f;
}

float HUD::GetClipSpaceY(float pos) const
{
  // 0..height -> 0..1 -> 1..-1
  return pos * m_rcp_viewport_height * -2.0f + 1.0f;
}

bool HUD::BeginDraw(u32 primitive_type)
{
#ifdef USE_MAPPED_VERTEX_BUFFER
  m_vertex_buffer_mapped_ptr = reinterpret_cast<char*>(m_vertex_buffer->Map(false, true));
  if (!m_vertex_buffer_mapped_ptr)
    return false;
#endif

  m_draw_primitive_type = primitive_type;
  return true;
}

void HUD::AddVertex2D(s32 x, s32 y, float u, float v, u32 color)
{
  AddVertex2D(static_cast<float>(x), static_cast<float>(y), u, v, color);
}

void HUD::AddVertex2D(float x, float y, float u, float v, u32 color)
{
  AddVertex2DClipSpace(GetClipSpaceX(x), GetClipSpaceY(y), u, v, color);
}

void HUD::AddVertex2DClipSpace(float x, float y, float u, float v, u32 color)
{
  if (m_draw_primitive_count == MAX_VERTICES_PER_DRAW)
    FlushDraw();

  HUDVertex vert = {{x, y, 0.0f}, {u, v}, color};
#ifdef USE_MAPPED_VERTEX_BUFFER
  assert(m_vertex_buffer_mapped_ptr);
  std::memcpy(m_vertex_buffer_mapped_ptr, &vert, sizeof(vert));
  m_vertex_buffer_mapped_ptr += sizeof(vert);
#else
  m_draw_vertices.push_back(vert);
#endif
  m_draw_primitive_count++;
}

void HUD::AddVertex3D(float x, float y, float z, float u, float v, u32 color)
{
  if (m_draw_primitive_count == MAX_VERTICES_PER_DRAW)
    FlushDraw();

  HUDVertex vert = {{x, y, z}, {u, v}, color};
#ifdef USE_MAPPED_VERTEX_BUFFER
  assert(m_vertex_buffer_mapped_ptr);
  std::memcpy(m_vertex_buffer_mapped_ptr, &vert, sizeof(vert));
  m_vertex_buffer_mapped_ptr += sizeof(vert);
#else
  m_draw_vertices.push_back(vert);
#endif
  m_draw_primitive_count++;
}

void HUD::FlushDraw()
{
  if (m_draw_primitive_count == 0)
    return;

  u32 current_primitive_type = m_draw_primitive_type;
  EndDraw();
  BeginDraw(current_primitive_type);
}

void HUD::EndDraw()
{
#ifdef USE_MAPPED_VERTEX_BUFFER
  m_vertex_buffer->Unmap();
  m_vertex_buffer_mapped_ptr = nullptr;
#else
  if (m_draw_primitive_count > 0)
  {
    m_vertex_buffer->Update(0, m_draw_primitive_count * sizeof(HUDVertex), m_draw_vertices.data());
    m_draw_vertices.clear();
  }
#endif

  if (m_draw_primitive_count > 0)
  {
    m_vertex_array->Bind();
    glDrawArrays(static_cast<GLenum>(m_draw_primitive_type), 0, m_draw_primitive_count);
    g_statistics->AddDraw();
    m_draw_primitive_count = 0;
  }

  m_draw_primitive_type = 0;
}
