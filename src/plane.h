#pragma once
#include <glm/glm.hpp>

class Plane
{
public:
  Plane();
  Plane(float a, float b, float c, float d);
  Plane(const glm::vec3& normal, float distance);

  const glm::vec3& GetNormal() const { return m_normal; }
  const float GetDistance() const { return m_distance; }

  void SetNormal(glm::vec3& norm) { m_normal = norm; }
  void SetDistance(float dist) { m_distance = dist; }

  glm::vec4 GetVec4() const;
  void SetVec4(const glm::vec4& vec);

  float Distance(float x, float y, float z) const;
  float Distance(const glm::vec3& pos) const;

  enum class Side
  {
    BehindPlane,
    OnPlane,
    InFrontOfPlane
  };

  Side ClassifyPoint(float x, float y, float z) const;
  Side ClassifyPoint(const glm::vec3& pos) const;

private:
  glm::vec3 m_normal;
  float m_distance;
};