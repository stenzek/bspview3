#include "pch.h"
#include "bsp.h"
#include "common.h"
#include <cstdio>

#pragma pack(push, 1)
struct BSP_LUMP
{
  int offset;
  int length;
};

struct BSP_HEADER
{
  char magic[4];
  int version;
  BSP_LUMP lumps[17];
};

struct BSP_TEXTURE_LUMP
{
  char name[64];
  int surface_flags;
  int contents_flags;
};

struct BSP_PLANE_LUMP
{
  float normal[3];
  float distance;
};

struct BSP_NODE_LUMP
{
  int plane;
  int children[2];
  int bbox_min[3];
  int bbox_max[3];
};

struct BSP_LEAF_LUMP
{
  int cluster;
  int area;
  int bbox_min[3];
  int bbox_max[3];
  int first_leaf_face;
  int num_leaf_faces;
  int first_leaf_brush;
  int num_leaf_brushes;
};

struct BSP_VERTEX_LUMP
{
  float position[3];
  float texcoords[2][2];
  float normal[3];
  unsigned char color[4];
};

struct BSP_FACE_LUMP
{
  int texture_index;
  int effect_index;
  int type;
  int first_vertex;
  int num_vertices;
  int first_mesh_vertex;
  int num_mesh_vertices;
  int lightmap_index;
  int lightmap_corner[2];
  int lightmap_size[2];
  float lightmap_origin[3];
  float lightmap_vecs[2][3];
  float normal[3];
  int patch_size[2];
};

struct BSP_LIGHTMAP_LUMP
{
  unsigned char data[128][128][3];
};

template<typename T, std::size_t N>
std::string GetArrayString(T const (&A)[N])
{
  std::string ret;
  ret.reserve(N);
  for (std::size_t i = 0; i < N; i++)
  {
    if (A[i] == '\0')
      break;

    ret.push_back(A[i]);
  }
  return ret;
}

BSP::BSP() {}

BSP::~BSP() {}

std::unique_ptr<BSP> BSP::Load(std::FILE* fp)
{
  IntermediateData idata;
  idata.fp = fp;

  unsigned file_size;
  BSP_HEADER header;
  if (std::fseek(fp, 0, SEEK_END) != 0 || (file_size = std::ftell(fp), std::fseek(fp, 0, SEEK_SET)) != 0 ||
      std::fread(&header, sizeof(header), 1, fp) != 1)
  {
    std::fprintf(stdout, "Failed to read BSP header\n");
    return nullptr;
  }

  // check all lump offsets
  for (int i = 0; i < NUM_LUMPS; i++)
  {
    if (unsigned(header.lumps[i].offset) > file_size ||
        unsigned(header.lumps[i].offset + header.lumps[i].length) > file_size)
    {
      std::fprintf(stdout, "Lump %d is out-of-range\n", i);
      return nullptr;
    }

    idata.lumps[i].offset = header.lumps[i].offset;
    idata.lumps[i].length = header.lumps[i].length;
  }

  std::unique_ptr<BSP> bsp(new BSP());

  if (!bsp->LoadIntermediateData(&idata))
    return nullptr;

  bsp->LoadTextures(&idata);
  bsp->LoadVertices(&idata);
  bsp->LoadIndices(&idata);
  bsp->LoadLightMaps(&idata);
  bsp->LoadFaces(&idata);
  bsp->LoadLeaves(&idata);
  bsp->LoadNodes(&idata);

  if (idata.load_error)
  {
    std::fprintf(stderr, "Failed to load BSP file\n");
    return nullptr;
  }

  bsp->TesselatePatches();

  return std::move(bsp);
}

template<typename ElementType>
std::vector<ElementType> BSP::LoadLump(IntermediateData* idata, LUMP lump)
{
  unsigned count = idata->lumps[lump].length / sizeof(ElementType);
  if ((idata->lumps[lump].length % sizeof(ElementType)) != 0)
  {
    std::fprintf(stderr, "Lump %u is unaligned", lump);
    return {};
  }

  if (std::fseek(idata->fp, idata->lumps[lump].offset, SEEK_SET) != 0)
  {
    std::fprintf(stderr, "Failed to seek to lump %u\n", lump);
    idata->load_error = true;
    return {};
  }

  std::vector<ElementType> ret;
  ret.resize(count);
  if (count > 0 && std::fread(ret.data(), sizeof(ElementType), count, idata->fp) != count)
  {
    std::fprintf(stderr, "Failed to read lump %u\n", lump);
    idata->load_error = true;
    return {};
  }

  return ret;
}

bool BSP::LoadIntermediateData(IntermediateData* idata)
{
  auto planes = LoadLump<BSP_PLANE_LUMP>(idata, LUMP_PLANES);
  idata->planes.resize(planes.size());
  for (size_t i = 0; i < planes.size(); i++)
  {
    const BSP_PLANE_LUMP& pin = planes[i];
    Plane& pout = idata->planes[i];
    pout.SetNormal(glm::vec3(pin.normal[0], pin.normal[1], pin.normal[2]));
    pout.SetDistance(pin.distance);
  }

  idata->leaf_faces = LoadLump<int>(idata, LUMP_LEAF_FACES);
  idata->leaf_brushes = LoadLump<int>(idata, LUMP_LEAF_BRUSHES);

  return !idata->load_error;
}

void BSP::LoadTextures(IntermediateData* idata)
{
  auto textures = LoadLump<BSP_TEXTURE_LUMP>(idata, LUMP_TEXTURES);
  m_textures.resize(textures.size());
  for (size_t i = 0; i < m_textures.size(); i++)
  {
    m_textures[i].name = GetArrayString(textures[i].name);
    m_textures[i].surface_flags = textures[i].surface_flags;
    m_textures[i].contents_flags = textures[i].contents_flags;
  }
}

void BSP::LoadVertices(IntermediateData* idata)
{
  auto vertices = LoadLump<BSP_VERTEX_LUMP>(idata, LUMP_VERTICES);
  m_vertices.resize(vertices.size());
  for (size_t i = 0; i < m_vertices.size(); i++)
  {
    const BSP_VERTEX_LUMP& vin = vertices[i];
    Vertex& vout = m_vertices[i];
    vout.position = glm::vec3(vin.position[0], vin.position[1], vin.position[2]);
    vout.normal = glm::vec3(vin.normal[0], vin.normal[1], vin.normal[2]);
    for (size_t j = 0; j < 2; j++)
      vout.texcoords[j] = glm::vec2(vin.texcoords[j][0], vin.texcoords[j][1]);
    for (size_t j = 0; j < 4; j++)
      vout.color_components[j] = vin.color[j];
  }
}

void BSP::LoadNodes(IntermediateData* idata)
{
  auto nodes = LoadLump<BSP_NODE_LUMP>(idata, LUMP_NODES);
  m_nodes.resize(nodes.size());
  for (size_t i = 0; i < m_nodes.size(); i++)
  {
    const BSP_NODE_LUMP& nin = nodes[i];
    Node& nout = m_nodes[i];

    if (unsigned(nin.plane) >= idata->planes.size())
    {
      std::fprintf(stderr, "Node %u has out-of-range plane\n", unsigned(i));
      idata->load_error = true;
      return;
    }

    nout.plane = idata->planes[nin.plane];
    for (size_t j = 0; j < 2; j++)
    {
      nout.children[j] = s32(nin.children[j]);
      if (nout.children[j] < 0)
      {
        if (unsigned(~nout.children[j]) >= m_leaves.size())
        {
          std::fprintf(stderr, "Node %u has out-of-range leaf child\n", unsigned(i));
          idata->load_error = true;
          return;
        }
      }
      else
      {
        if (unsigned(nout.children[j]) >= m_nodes.size())
        {
          std::fprintf(stderr, "Node %u has out-of-range node child\n", unsigned(i));
          idata->load_error = true;
          return;
        }
      }
    }

    nout.bbox_min = glm::vec3(float(nin.bbox_min[0]), float(nin.bbox_min[1]), float(nin.bbox_min[2]));
    nout.bbox_max = glm::vec3(float(nin.bbox_max[0]), float(nin.bbox_max[1]), float(nin.bbox_max[2]));
  }
}

void BSP::LoadLeaves(IntermediateData* idata)
{
  auto leaves = LoadLump<BSP_LEAF_LUMP>(idata, LUMP_LEAVES);
  m_leaves.resize(leaves.size());
  for (size_t i = 0; i < m_leaves.size(); i++)
  {
    const BSP_LEAF_LUMP& lin = leaves[i];
    Leaf& lout = m_leaves[i];

    lout.bbox_min = glm::vec3(float(lin.bbox_min[0]), float(lin.bbox_min[1]), float(lin.bbox_min[2]));
    lout.bbox_max = glm::vec3(float(lin.bbox_max[0]), float(lin.bbox_max[1]), float(lin.bbox_max[2]));
    lout.cluster = lin.cluster;
    lout.area = lin.area;

    if (lin.first_leaf_face < 0 || lin.num_leaf_faces < 0 ||
        unsigned(lin.first_leaf_face + lin.num_leaf_faces) > idata->leaf_faces.size() || lin.first_leaf_brush < 0 ||
        lin.num_leaf_brushes < 0 || unsigned(lin.first_leaf_brush + lin.num_leaf_brushes) > idata->leaf_brushes.size())
    {
      std::fprintf(stderr, "Leaf %u has out-of-range indices\n", u32(i));
      idata->load_error = true;
      return;
    }

    lout.faces.reserve(lin.num_leaf_faces);
    for (int i = 0; i < lin.num_leaf_faces; i++)
    {
      int index = idata->leaf_faces[lin.first_leaf_face + i];
      if (index < 0 || unsigned(index) >= m_faces.size())
      {
        std::fprintf(stderr, "Leaf %u has out-of-range indices\n", u32(i));
        idata->load_error = true;
        return;
      }

      lout.faces.push_back(index);
    }
  }
}

void BSP::LoadFaces(IntermediateData* idata)
{
  auto faces = LoadLump<BSP_FACE_LUMP>(idata, LUMP_FACES);
  m_faces.resize(faces.size());
  for (size_t i = 0; i < m_faces.size(); i++)
  {
    const BSP_FACE_LUMP& fin = faces[i];
    Face& fout = m_faces[i];
    fout.texture_index = fin.texture_index;
    fout.effect_index = fin.effect_index;
    fout.type = FACE_TYPE(fin.type);
    fout.base_vertex = fin.first_vertex;
    fout.num_vertices = fin.num_vertices;
    fout.base_index = fin.first_mesh_vertex;
    fout.num_indices = fin.num_mesh_vertices;
    fout.lightmap_index = fin.lightmap_index;
    fout.lightmap_corner[0] = fin.lightmap_corner[0];
    fout.lightmap_corner[1] = fin.lightmap_corner[1];
    fout.lightmap_size[0] = fin.lightmap_size[0];
    fout.lightmap_size[1] = fin.lightmap_size[1];
    fout.lightmap_origin = glm::vec3(fin.lightmap_origin[0], fin.lightmap_origin[1], fin.lightmap_origin[2]);
    for (size_t j = 0; j < 3; j++)
      fout.lightmap_vectors[j] = glm::vec3(fin.lightmap_vecs[j][0], fin.lightmap_vecs[j][1], fin.lightmap_vecs[j][2]);
    fout.normal = glm::vec3(fin.normal[0], fin.normal[1], fin.normal[2]);
    fout.patch_width = fin.patch_size[0];
    fout.patch_height = fin.patch_size[1];

    if (fin.first_mesh_vertex < 0 || fin.num_mesh_vertices < 0 ||
        unsigned(fin.first_mesh_vertex + fin.num_mesh_vertices) > m_indices.size())
    {
      std::fprintf(stderr, "Face %u has out-of-range indices\n", u32(i));
      idata->load_error = true;
      return;
    }

    if (fin.lightmap_index >= 0 && fin.lightmap_index >= int(m_lightmaps.size()))
    {
      std::fprintf(stderr, "Face %u has out-of-range lightmap texture: %d\n", u32(i), fin.lightmap_index);
      idata->load_error = true;
      return;
    }

    if (fin.texture_index >= 0 && fin.texture_index >= int(m_textures.size()))
    {
      std::fprintf(stderr, "Face %u has out-of-range texture: %d\n", u32(i), fin.texture_index);
      idata->load_error = true;
      return;
    }

    // fout.indices.resize(fin.num_mesh_vertices);
    // std::copy_n(idata->mesh_verts.begin() + fin.first_mesh_vertex, fin.num_mesh_vertices, fout.indices.begin());
  }
}

void BSP::TesselatePatches()
{
  auto InterpolateVertex = [](Vertex* vout, const Vertex* v0, const Vertex* v1, const Vertex* v2, float a, float b) {
#define INTERPOLATE(field) vout->field = v0->field * b * b + v1->field * 2.0f * b * a + v2->field * a * a
    INTERPOLATE(position);
    INTERPOLATE(texcoords[0]);
    INTERPOLATE(texcoords[1]);
    INTERPOLATE(normal);
#undef INTERPOLATE

    for (u32 i = 0; i < 4; i++)
    {
      vout->color_components[i] =
        u8(float(v0->color_components[i]) * b * b + float(v1->color_components[i]) * 2.0f * b * a +
           float(v2->color_components[i]) * a * a);
    }
  };

  // based on https://github.com/leezh/bspviewer/blob/master/src/bsp.cpp
  const int bezier_level = 3;

  for (Face& face : m_faces)
  {
    if (face.type != FACE_TYPE_PATCH)
      continue;

    const int expected_num_vertices = face.patch_width * face.patch_height;
    if (face.patch_width < 3 || face.patch_height < 3 || face.num_vertices < expected_num_vertices)
    {
      face.base_index = 0;
      face.num_indices = 0;
      continue;
    }

    const u32 patches_wide = (face.patch_width - 1) / 2;
    const u32 patches_high = (face.patch_height - 1) / 2;

    const u32 patch_vertex_count = u32(patches_wide * patches_high) * ((bezier_level + 1) * (bezier_level + 1));
    const u32 patch_index_count = u32(patches_wide * patches_high) * (bezier_level * bezier_level * 6);
    const u32 new_base_vertex = u32(m_vertices.size());
    const u32 new_base_index = u32(m_indices.size());
    m_vertices.resize(m_vertices.size() + patch_vertex_count);
    m_indices.resize(m_indices.size() + patch_index_count);

    u32 patch_base_vertex = new_base_vertex;
    u32 patch_base_index = new_base_index;

    for (u32 y = 0; y < patches_high; y++)
    {
      for (u32 x = 0; x < patches_wide; x++)
      {
        const Vertex* controls[9];
        const u32 control_start_offset = face.base_vertex + (face.patch_width * (y * 2)) + (x * 2);
        for (int c = 0; c < 3; c++)
        {
          const int offset = c * face.patch_width;
          controls[c * 3 + 0] = &m_vertices[control_start_offset + offset + 0];
          controls[c * 3 + 1] = &m_vertices[control_start_offset + offset + 1];
          controls[c * 3 + 2] = &m_vertices[control_start_offset + offset + 2];
        }

        int L1 = bezier_level + 1;
        for (int l = 0; l <= bezier_level; l++)
        {
          const float a = static_cast<float>(l) / float(bezier_level);
          const float b = 1.0f - a;
          InterpolateVertex(&m_vertices[patch_base_vertex + l], controls[0], controls[3], controls[6], a, b);
        }

        for (int l = 1; l <= bezier_level; l++)
        {
          const float a = static_cast<float>(l) / float(bezier_level);
          const float b = 1.0f - a;

          Vertex temp[3];
          for (int v = 0; v < 3; v++)
          {
            int offset = v * 3;
            InterpolateVertex(&temp[v], controls[offset + 0], controls[offset + 1], controls[offset + 2], a, b);
          }

          for (int v = 0; v <= bezier_level; v++)
          {
            const float aa = static_cast<float>(v) / float(bezier_level);
            const float bb = 1.0f - aa;

            InterpolateVertex(&m_vertices[patch_base_vertex + l * L1 + v], &temp[0], &temp[1], &temp[2], aa, bb);
          }
        }

        for (int ii = 0; ii < bezier_level; ii++)
        {
          for (int jj = 0; jj < bezier_level; jj++)
          {
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + ii * L1 + jj;
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + ii * L1 + (jj + 1);
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + (ii + 1) * L1 + (jj + 1);
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + (ii + 1) * L1 + (jj + 1);
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + (ii + 1) * L1 + jj;
            m_indices[patch_base_index++] = (patch_base_vertex - new_base_vertex) + ii * L1 + jj;
          }
        }

        patch_base_vertex += ((bezier_level + 1) * (bezier_level + 1));
      }
    }

    face.base_vertex = new_base_vertex;
    face.num_vertices = patch_vertex_count;
    face.base_index = new_base_index;
    face.num_indices = patch_index_count;
  }
}

void BSP::LoadIndices(IntermediateData* idata)
{
  m_indices = LoadLump<u32>(idata, LUMP_MESH_VERTICES);
  for (size_t i = 0; i < m_indices.size(); i++)
  {
    if (m_indices[i] >= m_vertices.size())
    {
      std::fprintf(stderr, "Index  %u has out-of-range indices\n", u32(i));
      idata->load_error = true;
      return;
    }
  }
}

void BSP::LoadLightMaps(IntermediateData* idata)
{
  auto lightmaps = LoadLump<BSP_LIGHTMAP_LUMP>(idata, LUMP_LIGHTMAPS);
  if (lightmaps.empty())
    return;

  m_lightmaps.resize(lightmaps.size());
  for (size_t i = 0; i < m_lightmaps.size(); i++)
  {
    const BSP_LIGHTMAP_LUMP& src = lightmaps[i];
    LightMap& dst = m_lightmaps[i];
    std::memcpy(dst.data, src.data, sizeof(dst.data));
  }
}
