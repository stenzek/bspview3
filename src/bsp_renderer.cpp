#include "pch.h"
#include "bsp_renderer.h"
#include "bsp.h"
#include "buffer.h"
#include "camera.h"
#include "resource_manager.h"
#include "shader.h"
#include "statistics.h"
#include "texture.h"
#include "vertex_array.h"

#pragma pack(push, 1)
struct BSPVertex
{
  float position[3];
  float texcoord[2][2];
  float normal[3];
  unsigned char color[4];
};
#pragma pack(pop)

static const VertexAttribute s_bsp_vertex_attributes[] = {
  {"in_position", GL_FLOAT, 3, 0, offsetof(BSPVertex, position), sizeof(BSPVertex), false},
  {"in_tex0", GL_FLOAT, 2, 0, offsetof(BSPVertex, texcoord[0]), sizeof(BSPVertex), false},
  {"in_tex1", GL_FLOAT, 2, 0, offsetof(BSPVertex, texcoord[1]), sizeof(BSPVertex), false},
  {"in_normal", GL_FLOAT, 3, 0, offsetof(BSPVertex, normal), sizeof(BSPVertex), false},
  {"in_color", GL_UNSIGNED_BYTE, 4, 0, offsetof(BSPVertex, color), sizeof(BSPVertex), true}};

BSPRenderer::BSPRenderer(const BSP* bsp) : m_bsp(bsp) {}

BSPRenderer::~BSPRenderer() {}

bool BSPRenderer::Initialize()
{
  return LoadTextures() && CreateLightmaps() && CreateShaders() && UploadVertices() && CreateRenderLeaves();
}

const VertexAttribute* BSPRenderer::GetBSPVertexAttributes()
{
  return s_bsp_vertex_attributes;
}

const size_t BSPRenderer::GetBSPVertexAttributeCount()
{
  return ARRAY_SIZE(s_bsp_vertex_attributes);
}

bool BSPRenderer::LoadTextures()
{
  m_textures.resize(m_bsp->GetTextureCount());
  for (size_t i = 0; i < m_bsp->GetTextureCount(); i++)
  {
    const BSP::Texture* tex = m_bsp->GetTexture(i);
    m_textures[i] = g_resource_manager->GetTexture(tex->name.c_str());
  }

  return true;
}

bool BSPRenderer::CreateLightmaps()
{
  m_lightmap_textures.resize(m_bsp->GetLightMapCount());
  for (size_t i = 0; i < m_lightmap_textures.size(); i++)
  {
    u8 lightmap_data[BSP::LIGHTMAP_SIZE][BSP::LIGHTMAP_SIZE][3];
    std::memcpy(lightmap_data, m_bsp->GetLightMap(i)->data, sizeof(lightmap_data));

    const u32 gamma_shift = 2;
    for (u32 y = 0; y < BSP::LIGHTMAP_SIZE; y++)
    {
      for (u32 x = 0; x < BSP::LIGHTMAP_SIZE; x++)
      {
        u32 r = u32(lightmap_data[y][x][0]);
        u32 g = u32(lightmap_data[y][x][1]);
        u32 b = u32(lightmap_data[y][x][2]);

        r <<= gamma_shift;
        g <<= gamma_shift;
        b <<= gamma_shift;

        u32 max = std::max(r, std::max(g, b));
        if (max > 255)
        {
          float f = 255.0f / static_cast<float>(max);
          r = static_cast<u32>(f * static_cast<float>(r));
          g = static_cast<u32>(f * static_cast<float>(g));
          b = static_cast<u32>(f * static_cast<float>(b));
        }

        lightmap_data[y][x][0] = u8(r);
        lightmap_data[y][x][1] = u8(g);
        lightmap_data[y][x][2] = u8(b);
      }
    }

    m_lightmap_textures[i] = Texture::Create(Texture::Format::FORMAT_RGB8, BSP::LIGHTMAP_SIZE, BSP::LIGHTMAP_SIZE, 1,
                                             lightmap_data, true, false, false);
    if (!m_lightmap_textures[i])
      return false;
  }

  m_default_lightmap_texture = Texture::CreateSingleColorTexture(0xFFFFFFFF, 1, 1);
  if (!m_default_lightmap_texture)
    return false;

  return true;
}

bool BSPRenderer::UploadVertices()
{
  std::vector<BSPVertex> vertices;
  for (size_t i = 0; i < m_bsp->GetVertexCount(); i++)
  {
    const BSP::Vertex* vin = m_bsp->GetVertex(i);
    BSPVertex vout;
    std::memcpy(&vout.position, &vin->position, sizeof(vout.position));
    std::memcpy(&vout.texcoord[0], &vin->texcoords[0], sizeof(vout.texcoord[0]));
    std::memcpy(&vout.texcoord[1], &vin->texcoords[1], sizeof(vout.texcoord[1]));
    std::memcpy(&vout.normal, &vin->normal, sizeof(vout.normal));
    std::memcpy(&vout.color, &vin->color_components, sizeof(vout.color));
    vertices.push_back(vout);
  }

  m_vertex_buffer =
    Buffer::Create(Buffer::Type::VertexBuffer, sizeof(BSPVertex) * vertices.size(), vertices.data(), false);
  if (!m_vertex_buffer)
    return false;

  const Buffer* bsp_vertex_buffers[1] = {m_vertex_buffer.get()};
  m_vertex_array = VertexArray::Create(
    s_bsp_vertex_attributes, sizeof(s_bsp_vertex_attributes) / sizeof(s_bsp_vertex_attributes[0]), bsp_vertex_buffers);

  if (!m_vertex_array)
    return false;

  return true;
}

bool BSPRenderer::CreateRenderLeaves()
{
  std::vector<u32> indices;

  for (size_t i = 0; i < m_bsp->GetLeafCount(); i++)
  {
    const BSP::Leaf* leaf = m_bsp->GetLeaf(i);
    m_render_leaves.push_back(CreateRenderLeaf(leaf, indices));
  }

  m_index_buffer = Buffer::Create(Buffer::Type::IndexBuffer, sizeof(u32) * indices.size(), indices.data(), false);
  if (!m_index_buffer)
    return false;

  return true;
}

static bool CanRenderFace(const BSP::Face* face)
{
  return (face->num_indices > 0);
}

static bool CanMergeFaces(const BSP::Face* lhs, const BSP::Face* rhs)
{
  return (lhs->texture_index == rhs->texture_index && lhs->effect_index == rhs->effect_index &&
          lhs->lightmap_index == rhs->lightmap_index);
}

BSPRenderer::RenderLeaf BSPRenderer::CreateRenderLeaf(const BSP::Leaf* leaf, std::vector<u32>& indices) const
{
  RenderLeaf rleaf;
  rleaf.bbox_min = leaf->bbox_min;
  rleaf.bbox_max = leaf->bbox_max;

  for (size_t i = 0; i < leaf->faces.size(); i++)
  {
    const BSP::Face* face = m_bsp->GetFace(leaf->faces[i]);
    if (!CanRenderFace(face))
      continue;

    // Skip those which have already been processed.
    bool done = false;
    for (size_t j = 0; j < i; j++)
    {
      const BSP::Face* other_face = m_bsp->GetFace(leaf->faces[j]);
      if (CanRenderFace(other_face) && CanMergeFaces(face, other_face))
      {
        // Already done in the other direction.
        done = true;
        break;
      }
    }
    if (done)
      continue;

    // Add this face.
    RenderLeaf::Batch batch;
    batch.material_index = face->texture_index;
    batch.lightmap_index = face->lightmap_index;
    batch.start_index = u32(indices.size());
    batch.num_indices = 0;

    for (int offset = 0; offset < face->num_indices; offset++)
    {
      indices.push_back(u32(face->base_vertex) + m_bsp->GetIndex(face->base_index + offset));
      batch.num_indices++;
    }

    // Add other matching faces.
    for (size_t j = i; j < leaf->faces.size(); j++)
    {
      const BSP::Face* other_face = m_bsp->GetFace(leaf->faces[j]);
      if (!CanRenderFace(other_face) || !CanMergeFaces(face, other_face))
        continue;

      for (int offset = 0; offset < other_face->num_indices; offset++)
      {
        indices.push_back(u32(other_face->base_vertex) + m_bsp->GetIndex(other_face->base_index + offset));
        batch.num_indices++;
      }
    }

    rleaf.batches.push_back(std::move(batch));
  }

  return rleaf;
}

std::unique_ptr<ShaderProgram> CreateProgram()
{
  const char* vs = R"(
#version 430

uniform mat4 projection;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex0;
layout(location = 2) in vec2 in_tex1;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec4 in_color;

layout(location = 0) out vec2 v_tex0;
layout(location = 1) out vec2 v_tex1;
layout(location = 2) out vec3 v_normal;
layout(location = 3) out vec4 v_color;

void main()
{
  gl_Position = projection * vec4(in_position, 1.0);
  v_tex0 = in_tex0;
  v_tex1 = in_tex1;
  v_normal = in_normal;
  v_color = in_color;
}
)";

  const char* fs = R"(
#version 430

layout(binding = 0) uniform sampler2D samp0;
layout(binding = 1) uniform sampler2D samp1;

layout(location = 0) in vec2 v_tex0;
layout(location = 1) in vec2 v_tex1;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec4 v_color;

layout(location = 0) out vec4 ocol0;

void main()
{
  vec4 tex_color = texture(samp0, v_tex0);
  vec4 lightmap_color = texture(samp1, v_tex1);
  ocol0 = tex_color;

  ocol0.rgb *= lightmap_color.rgb;
  //ocol0.rgb *= min(lightmap_color.rgb * 2.5, vec3(1.0, 1.0, 1.0));
  //ocol0.rgb *= min(pow(lightmap_color.rgb, vec3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)), vec3(1.0, 1.0, 1.0));
}
)";

  static const char* uniform_names[] = {"projection"};

  auto vertex_shader = Shader::Create(GL_VERTEX_SHADER, vs, std::strlen(vs));
  auto fragment_shader = Shader::Create(GL_FRAGMENT_SHADER, fs, std::strlen(fs));
  if (!vertex_shader || !fragment_shader)
    return nullptr;

  auto program =
    ShaderProgram::Create(s_bsp_vertex_attributes, ARRAY_SIZE(s_bsp_vertex_attributes), vertex_shader.get(),
                          fragment_shader.get(), 2, 1, uniform_names, ARRAY_SIZE(uniform_names));
  if (!program)
    return nullptr;

  return program;
}

bool BSPRenderer::CreateShaders()
{
  m_lightmap_shader_program = CreateProgram();
  if (!m_lightmap_shader_program)
    return false;

  return true;
}

#if 0
int FindNodeIndex(const glm::vec3& pos)
{
  int index = 0;
  while (index >= 0)
  {
    const auto& node = m_bsp->GetNode(index);
    if (glm::dot(node.plane, pos) >= plane.mDistance)
      index = node.mChildren[0];
    else
      index = node.mChildren[1];
  }

  return ~index;
}
#endif

void BSPRenderer::Render(const Camera& camera) const
{
  m_lightmap_shader_program->Bind();
  m_lightmap_shader_program->SetUniform(0, camera.GetViewProjectionMatrix());

  m_vertex_array->Bind();
  m_index_buffer->Bind();

#if 0
  DrawNode(camera, m_bsp->GetRootNode());
#else
  for (const RenderLeaf& leaf : m_render_leaves)
    DrawLeaf(camera, leaf);
#endif
}

void BSPRenderer::DrawNode(const Camera& camera, const BSP::Node* node) const
{
  if (!camera.GetFrustum().IntersectsAABox(node->bbox_min, node->bbox_max))
    return;

  // order of traversal is reversed for solid vs transparent
  const Plane::Side side = node->plane.ClassifyPoint(camera.GetPosition());
  const u32 first_child = (side == Plane::Side::BehindPlane) ? 1 : 0;
  const u32 second_child = first_child ^ 1u;
  if (node->children[first_child] < 0)
    DrawLeaf(camera, m_render_leaves[~node->children[first_child]]);
  else
    DrawNode(camera, m_bsp->GetNode(node->children[first_child]));

  if (node->children[second_child] < 0)
    DrawLeaf(camera, m_render_leaves[~node->children[second_child]]);
  else
    DrawNode(camera, m_bsp->GetNode(node->children[second_child]));
}

void BSPRenderer::DrawLeaf(const Camera& camera, const RenderLeaf& leaf) const
{
  if (!camera.GetFrustum().IntersectsAABox(leaf.bbox_min, leaf.bbox_max))
    return;

  for (const RenderLeaf::Batch& batch : leaf.batches)
  {
    if (batch.material_index >= 0 && m_textures[batch.material_index])
      m_textures[batch.material_index]->Bind(0);
    else
      g_resource_manager->GetDefaultTexture()->Bind(0);

    if (batch.lightmap_index >= 0)
      m_lightmap_textures[batch.lightmap_index]->Bind(1);
    else
      m_default_lightmap_texture->Bind(1);

    glDrawElements(GL_TRIANGLES, batch.num_indices, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(batch.start_index * sizeof(u32)));
    g_statistics->AddDraw();
  }
}
