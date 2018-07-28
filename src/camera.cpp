#include "pch.h"
#include "camera.h"

Camera::Camera(const glm::vec3& forward_vector /*= glm::vec3(0.0f, 1.0f, 0.0f)*/) : m_forward_vector(forward_vector)
{
  // SetRotationFromPitchYaw -> UpdateViewMatrix -> UpdateViewProjectionMatrix
  UpdateProjectionMatrix();
  SetRotationFromPitchYaw();
}

void Camera::SetPosition(const glm::vec3& pos)
{
  m_position = pos;
  UpdateViewMatrix();
}

void Camera::SetRotation(const glm::quat& rotation)
{
  m_rotation = glm::normalize(rotation);
  UpdateViewMatrix();
}

void Camera::SetPitch(float pitch)
{
  m_pitch = glm::clamp(pitch, -90.0f, 90.0f);
  SetRotationFromPitchYaw();
}

void Camera::SetYaw(float yaw)
{
  m_yaw = yaw;
  while (m_yaw < 0.0f)
    m_yaw += 360.0f;
  while (m_yaw >= 360.0f)
    m_yaw -= 360.0f;

  SetRotationFromPitchYaw();
}

void Camera::SetRotationFromPitchYaw()
{
  m_rotation = glm::quat(glm::vec3(glm::radians(m_pitch), 0.0f, glm::radians(m_yaw)));
  UpdateViewMatrix();
}

void Camera::SetMoveVector(const glm::vec3& move_vector)
{
  m_move_vector = glm::clamp(move_vector, glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
}

void Camera::SetAspectRatio(u32 viewport_width, u32 viewport_height)
{
  u32 width = std::max(viewport_width, 1u);
  u32 height = std::max(viewport_height, 1u);
  SetAspectRatio(float(width) / float(height));
}

void Camera::SetAspectRatio(float value)
{
  m_aspect_ratio = value;
  UpdateProjectionMatrix();
}

void Camera::SetFieldOfView(float value)
{
  m_field_of_view = value;
  UpdateProjectionMatrix();
}

void Camera::UpdateViewMatrix()
{
  static const glm::mat4 camera_zup_to_yup(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 0.0f, 1.0f);

  glm::mat4 rotation = glm::toMat4(glm::inverse(m_rotation));
  glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
  m_view_matrix = camera_zup_to_yup * rotation * translation;
  UpdateViewProjectionMatrix();
}

void Camera::UpdateProjectionMatrix()
{
  // TODO: Customizable znear/far
  m_projection_matrix = glm::perspective(m_field_of_view, m_aspect_ratio, 1.0f, 10000.0f);
  UpdateViewProjectionMatrix();
}

void Camera::UpdateViewProjectionMatrix()
{
  m_view_projection_matrix = m_projection_matrix * m_view_matrix;
  m_frustum.Set(m_view_projection_matrix);
}
void Camera::Update(float time_diff)
{
  if (m_move_vector.x != 0.0f || m_move_vector.y != 0.0f)
  {
    float speed = m_move_speed;
    if (m_turbo)
      speed *= m_turbo_coefficient;

    m_position += m_rotation * (m_move_vector * (speed * time_diff));
    UpdateViewMatrix();
  }
}
