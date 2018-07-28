#include "pch.h"
#include "statistics.h"

static Statistics s_statistics;
Statistics* g_statistics = &s_statistics;

Statistics::Statistics() : m_last_fps_time(ClockSource::now()) {}

Statistics::~Statistics() = default;

void Statistics::BeginFrame()
{
  m_frame_start_time = ClockSource::now();
}

void Statistics::EndFrame()
{
  auto now = ClockSource::now();

  float frame_time = std::chrono::duration<float>(now - m_frame_start_time).count();
  m_this_frame.frame_time = frame_time;
  m_last_frame = m_this_frame;

  m_fps_frames_rendered++;
  float time_since_last_fps = std::chrono::duration<float>(now - m_last_fps_time).count();
  if (time_since_last_fps > 0.1f)
  {
    m_last_fps = static_cast<float>(m_fps_frames_rendered) / time_since_last_fps;
    m_fps_frames_rendered = 0;
    m_last_fps_time = now;
  }

  m_this_frame.num_draws = 0;
  m_this_frame.frame_time = 0.0f;
}
