#pragma once
#include "common.h"
#include <chrono>

class Statistics
{
public:
  Statistics();
  ~Statistics();

  u32 GetLastFrameNumDraws() const { return m_last_frame.num_draws; }
  float GetLastFrameTime() const { return m_last_frame.frame_time; }
  float GetLastFPS() const { return m_last_fps; }

  void BeginFrame();
  void EndFrame();

  void AddDraw() { m_this_frame.num_draws++; }

private:
  using ClockSource = std::chrono::steady_clock;

  struct Stats
  {
    u32 num_draws = 0;
    float frame_time = 0.0f;
  };

  Stats m_last_frame;
  Stats m_this_frame;

  float m_last_fps = 0.0f;
  u32 m_fps_frames_rendered = 0;

  ClockSource::time_point m_last_fps_time;

  ClockSource::time_point m_frame_start_time;
};

extern Statistics* g_statistics;