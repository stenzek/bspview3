#pragma once
#include <glm/glm.hpp>

class Frustum
{
public:
  Frustum();
  Frustum(const glm::mat4& view_proj_matrix);

  void Set(const glm::mat4& view_proj_matrix);

  bool Intersects(const glm::vec3& pos) const;

  bool IntersectsAABox(const glm::vec3& bmin, const glm::vec3& bmax) const;

private:
  enum PLANE : unsigned
  {
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_TOP,
    PLANE_BOTTOM,
    PLANE_NEAR,
    PLANE_FAR,
    NUM_PLANES
  };
  glm::vec4 m_planes[NUM_PLANES];
};
