#include "pch.h"
#include "plane.h"

Plane::Plane() : m_normal(0.0f, 0.0f, 1.0f), m_distance(0.0f) {}

Plane::Plane(float a, float b, float c, float d) : m_normal(a, b, c), m_distance(d) {}

Plane::Plane(const glm::vec3& normal, float distance) : m_normal(normal), m_distance(distance) {}

glm::vec4 Plane::GetVec4() const
{
  return glm::vec4(m_normal.x, m_normal.y, m_normal.z, m_distance);
}

void Plane::SetVec4(const glm::vec4& vec)
{
  m_normal = glm::vec3(vec.x, vec.y, vec.z);
  m_distance = vec.w;
}

float Plane::Distance(float x, float y, float z) const
{
  return Distance(glm::vec3(x, y, z));
}

float Plane::Distance(const glm::vec3& pos) const
{
  return glm::dot(m_normal, pos) + m_distance;
}

Plane::Side Plane::ClassifyPoint(float x, float y, float z) const
{
  return ClassifyPoint(glm::vec3(x, y, z));
}

Plane::Side Plane::ClassifyPoint(const glm::vec3& pos) const
{
  const float epsilon = 0.001f;
  const float dist = Distance(pos);
  if (dist <= -epsilon)
    return Side::BehindPlane;
  else if (dist <= epsilon)
    return Side::OnPlane;
  else
    return Side::InFrontOfPlane;
}
