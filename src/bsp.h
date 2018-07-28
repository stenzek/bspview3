#pragma once
#include "common.h"
#include "plane.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class BSP
{
public:
  enum : u32
  {
    LIGHTMAP_SIZE = 128
  };

  enum FACE_TYPE
  {
    FACE_TYPE_NONE,
    FACE_TYPE_BRUSH,
    FACE_TYPE_PATCH,
    FACE_TYPE_MESH
  };

  enum LUMP
  {
    LUMP_ENTITIES,
    LUMP_TEXTURES,
    LUMP_PLANES,
    LUMP_NODES,
    LUMP_LEAVES,
    LUMP_LEAF_FACES,
    LUMP_LEAF_BRUSHES,
    LUMP_MODELS,
    LUMP_BRUSHES,
    LUMP_BRUSH_SIDES,
    LUMP_VERTICES,
    LUMP_MESH_VERTICES,
    LUMP_EFFECTS,
    LUMP_FACES,
    LUMP_LIGHTMAPS,
    LUMP_LIGHTVOLS,
    LUMP_VISDATA,
    NUM_LUMPS
  };

  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords[2];
    union
    {
      unsigned char color_components[4];
      unsigned int color_uint;
    };
  };

  struct Texture
  {
    std::string name;
    int surface_flags;
    int contents_flags;
  };

  struct Brush
  {
    struct Side
    {
      Plane plane;
      int texture_index;
    };

    std::vector<Side> sides;
    int texture_index;
  };

  struct Face
  {
    int texture_index;
    int effect_index;
    FACE_TYPE type;

    int base_vertex;
    int num_vertices;

    int base_index;
    int num_indices;

    int lightmap_index;
    int lightmap_corner[2];
    int lightmap_size[2];

    glm::vec3 lightmap_origin;
    glm::vec3 lightmap_vectors[2];
    glm::vec3 normal;

    int patch_width;
    int patch_height;
  };

  struct Model
  {
    std::vector<u32> faces;
    std::vector<u32> brushes;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
  };

  struct Leaf
  {
    std::vector<u32> faces;
    std::vector<u32> brushes;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    int cluster;
    int area;
  };

  struct Node
  {
    // if children[i] < 0, then is leaf, index=~children[i], else node
    Plane plane;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    s32 children[2];
  };

  struct Effect
  {
    std::string shader_name;
    int brush_index;
    int unk;
  };

  struct LightMap
  {
    u8 data[LIGHTMAP_SIZE][LIGHTMAP_SIZE][3];
  };

  ~BSP();

  static std::unique_ptr<BSP> Load(std::FILE* fp);

  size_t GetTextureCount() const { return m_textures.size(); }
  const Texture* GetTexture(size_t i) const { return &m_textures[i]; }
  const std::vector<Texture>& GetTextures() const { return m_textures; }

  size_t GetVertexCount() const { return m_vertices.size(); }
  const Vertex* GetVertex(size_t i) const { return &m_vertices[i]; }
  const std::vector<Vertex>& GetVertices() const { return m_vertices; }

  size_t GetIndexCount() const { return m_indices.size(); }
  const u32 GetIndex(size_t i) const { return m_indices[i]; }
  const std::vector<u32>& GetIndices() const { return m_indices; }

  size_t GetNodeCount() const { return m_nodes.size(); }
  const Node* GetNode(size_t i) const { return &m_nodes[i]; }
  const Node* GetRootNode() const { return &m_nodes[0]; }
  const std::vector<Node>& GetNodes() const { return m_nodes; }

  size_t GetLeafCount() const { return m_leaves.size(); }
  const Leaf* GetLeaf(size_t i) const { return &m_leaves[i]; }
  const std::vector<Leaf>& GetLeaves() const { return m_leaves; }

  size_t GetFaceCount() const { return m_faces.size(); }
  const Face* GetFace(size_t i) const { return &m_faces[i]; }
  const std::vector<Face>& GetFaces() const { return m_faces; }

  size_t GetLightMapCount() const { return m_lightmaps.size(); }
  const LightMap* GetLightMap(size_t i) const { return &m_lightmaps[i]; }
  const std::vector<LightMap>& GetLightMaps() const { return m_lightmaps; }

private:
  struct IntermediateData
  {
    struct Lump
    {
      unsigned offset;
      unsigned length;
    };
    std::FILE* fp = nullptr;
    Lump lumps[NUM_LUMPS] = {};
    std::vector<Plane> planes;
    std::vector<int> leaf_faces;
    std::vector<int> leaf_brushes;
    bool load_error = false;
  };

  BSP();

  template<typename ElementType>
  std::vector<ElementType> LoadLump(IntermediateData* idata, LUMP lump);

  bool LoadIntermediateData(IntermediateData* idata);

  void LoadTextures(IntermediateData* idata);
  void LoadVertices(IntermediateData* idata);
  void LoadIndices(IntermediateData* idata);
  void LoadNodes(IntermediateData* idata);
  void LoadLeaves(IntermediateData* idata);
  void LoadFaces(IntermediateData* idata);
  void LoadLightMaps(IntermediateData* idata);

  void TesselatePatches();

  std::vector<Texture> m_textures;
  std::vector<Vertex> m_vertices;
  std::vector<u32> m_indices;
  std::vector<Node> m_nodes;
  std::vector<Leaf> m_leaves;
  std::vector<Face> m_faces;
  std::vector<LightMap> m_lightmaps;
};
