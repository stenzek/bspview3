#include "pch.h"
#include "frustum.h"

Frustum::Frustum()
{
  Set(glm::mat4(1.0f));
}

Frustum::Frustum(const glm::mat4& view_proj_matrix)
{
  Set(view_proj_matrix);
}

void Frustum::Set(const glm::mat4& view_proj_matrix)
{
  m_planes[PLANE_LEFT].x = view_proj_matrix[0][3] + view_proj_matrix[0][0];
  m_planes[PLANE_LEFT].y = view_proj_matrix[1][3] + view_proj_matrix[1][0];
  m_planes[PLANE_LEFT].z = view_proj_matrix[2][3] + view_proj_matrix[2][0];
  m_planes[PLANE_LEFT].w = view_proj_matrix[3][3] + view_proj_matrix[3][0];

  m_planes[PLANE_RIGHT].x = view_proj_matrix[0][3] - view_proj_matrix[0][0];
  m_planes[PLANE_RIGHT].y = view_proj_matrix[1][3] - view_proj_matrix[1][0];
  m_planes[PLANE_RIGHT].z = view_proj_matrix[2][3] - view_proj_matrix[2][0];
  m_planes[PLANE_RIGHT].w = view_proj_matrix[3][3] - view_proj_matrix[3][0];

  m_planes[PLANE_TOP].x = view_proj_matrix[0][3] - view_proj_matrix[0][1];
  m_planes[PLANE_TOP].y = view_proj_matrix[1][3] - view_proj_matrix[1][1];
  m_planes[PLANE_TOP].z = view_proj_matrix[2][3] - view_proj_matrix[2][1];
  m_planes[PLANE_TOP].w = view_proj_matrix[3][3] - view_proj_matrix[3][1];

  m_planes[PLANE_BOTTOM].x = view_proj_matrix[0][3] + view_proj_matrix[0][1];
  m_planes[PLANE_BOTTOM].y = view_proj_matrix[1][3] + view_proj_matrix[1][1];
  m_planes[PLANE_BOTTOM].z = view_proj_matrix[2][3] + view_proj_matrix[2][1];
  m_planes[PLANE_BOTTOM].w = view_proj_matrix[3][3] + view_proj_matrix[3][1];

  m_planes[PLANE_NEAR].x = view_proj_matrix[0][3] + view_proj_matrix[0][2];
  m_planes[PLANE_NEAR].y = view_proj_matrix[1][3] + view_proj_matrix[1][2];
  m_planes[PLANE_NEAR].z = view_proj_matrix[2][3] + view_proj_matrix[2][2];
  m_planes[PLANE_NEAR].w = view_proj_matrix[3][3] + view_proj_matrix[3][2];

  m_planes[PLANE_FAR].x = view_proj_matrix[0][3] - view_proj_matrix[0][2];
  m_planes[PLANE_FAR].y = view_proj_matrix[1][3] - view_proj_matrix[1][2];
  m_planes[PLANE_FAR].z = view_proj_matrix[2][3] - view_proj_matrix[2][2];
  m_planes[PLANE_FAR].w = view_proj_matrix[3][3] - view_proj_matrix[3][2];
}

bool Frustum::Intersects(const glm::vec3& pos) const
{
  for (unsigned i = 0; i < NUM_PLANES; i++)
  {
    const glm::vec4& plane = m_planes[i];
    if ((glm::dot(glm::vec3(plane.x, plane.y, plane.z), pos) + plane.w) < 0.0f)
      return false;
  }

  return true;
}

bool Frustum::IntersectsAABox(const glm::vec3& bmin, const glm::vec3& bmax) const
{
  const glm::vec3 box_verts[8] = {glm::vec3(bmin.x, bmin.y, bmin.z), glm::vec3(bmax.x, bmin.y, bmin.z),
                                  glm::vec3(bmin.x, bmax.y, bmin.z), glm::vec3(bmax.x, bmax.y, bmin.z),
                                  glm::vec3(bmin.x, bmin.y, bmax.z), glm::vec3(bmax.x, bmin.y, bmax.z),
                                  glm::vec3(bmin.x, bmax.y, bmax.z), glm::vec3(bmax.x, bmax.y, bmax.z)};

  for (unsigned i = 0; i < NUM_PLANES; i++)
  {
    const glm::vec4& plane = m_planes[i];
    const glm::vec3 normal(plane.x, plane.y, plane.z);
    for (unsigned j = 0; j < sizeof(box_verts) / sizeof(box_verts[0]); j++)
    {
      if ((glm::dot(normal, box_verts[j]) + plane.w) >= 0.0f)
        goto next_plane;
    }

    // all vertices outside of plane
    return false;

  next_plane:
    continue;
  }

  return true;
}
