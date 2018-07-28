#pragma once
#include "bsp.h"
#include <memory>
#include <vector>

class Buffer;
class Camera;
class ShaderProgram;
struct VertexAttribute;
class VertexArray;
class Texture;

class BSPRenderer
{
public:
  BSPRenderer(const BSP* bsp);
  ~BSPRenderer();

  bool Initialize();

  void Render(const Camera& camera) const;

  static const VertexAttribute* GetBSPVertexAttributes();
  static const size_t GetBSPVertexAttributeCount();

private:
  struct RenderLeaf
  {
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;

    struct Batch
    {
      s32 material_index;
      s32 lightmap_index;
      u32 start_index;
      u32 num_indices;
    };

    std::vector<Batch> batches;
  };

  bool LoadTextures();
  bool CreateLightmaps();

  bool CreateShaders();

  bool UploadVertices();

  bool CreateRenderLeaves();
  RenderLeaf CreateRenderLeaf(const BSP::Leaf* leaf, std::vector<u32>& indices) const;

  void DrawNode(const Camera& camera, const BSP::Node* node) const;
  void DrawLeaf(const Camera& camera, const RenderLeaf& leaf) const;
  void DrawNodeBounds(const Camera& camera) const;

  const BSP* m_bsp;

  std::unique_ptr<Buffer> m_vertex_buffer;
  std::unique_ptr<Buffer> m_index_buffer;
  std::unique_ptr<VertexArray> m_vertex_array;

  std::vector<const Texture*> m_textures;
  std::vector<std::unique_ptr<Texture>> m_lightmap_textures;
  std::unique_ptr<Texture> m_default_lightmap_texture;

  std::unique_ptr<ShaderProgram> m_lightmap_shader_program;

  std::vector<RenderLeaf> m_render_leaves;
};
