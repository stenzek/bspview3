#pragma once
#include "frustum.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// TODO: "move directions", for time + controller axis
class Camera
{
public:
  Camera(const glm::vec3& forward_vector = glm::vec3(0.0f, 1.0f, 0.0f));

  const glm::vec3& GetPosition() const { return m_position; }
  const glm::quat& GetRotation() const { return m_rotation; }
  const float GetPitch() const { return m_pitch; }
  const float GetYaw() const { return m_yaw; }

  void SetPosition(const glm::vec3& pos);
  void SetRotation(const glm::quat& rotation);
  void SetPitch(float pitch);
  void SetYaw(float yaw);

  void ModPitch(float amount) { SetPitch(GetPitch() + amount); }
  void ModYaw(float amount) { SetYaw(GetYaw() + amount); }

  // Moves the camera if direction is set.
  void Update(float time_diff);

  const glm::vec3& GetMoveVector() const { return m_move_vector; }
  void ModViewVector(const glm::vec3& amount) { SetMoveVector(m_move_vector + amount); }
  void SetMoveVector(const glm::vec3& move_vector);

  float GetAspectRatio() const { return m_aspect_ratio; }
  float GetFieldOfView() const { return m_field_of_view; }
  void SetAspectRatio(u32 viewport_width, u32 viewport_height);
  void SetAspectRatio(float value);
  void SetFieldOfView(float value);

  const glm::mat4& GetViewMatrix() const { return m_view_matrix; }
  const glm::mat4& GetProjectionMatrix() const { return m_projection_matrix; }
  const glm::mat4& GetViewProjectionMatrix() const { return m_view_projection_matrix; }

  const Frustum& GetFrustum() const { return m_frustum; }

  float GetMoveSpeed() const { return m_move_speed; }
  void SetMoveSpeed(float speed) { m_move_speed = speed; }
  float GetTurboCoefficient() const { return m_turbo_coefficient; }
  void SetTurboCoefficient(float coeff) { m_turbo_coefficient = coeff; }
  bool IsTurboEnabled() const { return m_turbo; }
  void SetTurboEnabled(bool turbo) { m_turbo = turbo; }

private:
  void SetRotationFromPitchYaw();
  void UpdateViewMatrix();
  void UpdateProjectionMatrix();
  void UpdateViewProjectionMatrix();

  glm::vec3 m_forward_vector;

  glm::vec3 m_position{0.0f, 0.0f, 0.0f};
  glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};
  float m_pitch;
  float m_yaw;

  glm::vec3 m_move_vector{0.0f, 0.0f, 0.0f};

  float m_aspect_ratio = 640.0f / 480.0f;
  float m_field_of_view = 55.0f;

  glm::mat4 m_view_matrix;
  glm::mat4 m_projection_matrix;
  glm::mat4 m_view_projection_matrix;

  Frustum m_frustum;
  float m_move_speed = 350.0f;
  float m_turbo_coefficient = 4.0f;
  bool m_turbo = false;
};
