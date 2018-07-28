#include "pch.h"
#include "bsp.h"
#include "bsp_renderer.h"
#include "camera.h"
#include "colors.h"
#include "common.h"
#include "font.h"
#include "glad.h"
#include "hud.h"
#include "resource_manager.h"
#include "statistics.h"
#include "util.h"
#include <SDL/SDL.h>
#include <sys/stat.h>

u32 g_num_draws = 0;

static std::unique_ptr<BSP> s_bsp;
static std::unique_ptr<BSPRenderer> s_bsp_renderer;
static std::unique_ptr<Font> s_font;
static SDL_Window* s_window;
static Camera s_camera;
static bool s_mouse_captured = false;
static std::chrono::steady_clock::time_point s_last_frame_time;

namespace {

void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam)
{
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;

  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

bool Setup()
{
  std::printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

  int red_size, green_size, blue_size, depth_size;
  SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red_size);
  SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green_size);
  SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue_size);
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth_size);
  std::printf("Backbuffer: Red=%d, Green=%d, Blue=%d, Depth=%d\n", red_size, green_size, blue_size, depth_size);

  if (glDebugMessageCallback)
  {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, nullptr);
  }

  if (!g_resource_manager->Initialize())
    return false;

  s_bsp_renderer = std::make_unique<BSPRenderer>(s_bsp.get());
  if (!s_bsp_renderer->Initialize())
    return false;

  s_font = Font::Create();
  if (!s_font)
    return false;

  if (!g_hud->Initialize())
    return false;

  s_last_frame_time = std::chrono::high_resolution_clock::now();
  return true;
}

void Render()
{
  g_statistics->BeginFrame();

  auto time_now = std::chrono::steady_clock::now();
  float time_diff = std::chrono::duration<float>(time_now - s_last_frame_time).count();
  s_last_frame_time = time_now;
  g_num_draws = 0;

  s_camera.Update(time_diff);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0, 0, 640, 480);
  glDepthRange(0.0, 1.0);

  g_hud->SetViewportSize(640, 480);

  s_bsp_renderer->Render(s_camera);

  s_font->RenderFormattedText(4, 4, 640, 480, Colors::White, "%u draws", g_statistics->GetLastFrameNumDraws());
  s_font->RenderFormattedText(640 - 256, 4, 640, 480, Colors::White, "%.2f fps (%.2f ms)", g_statistics->GetLastFPS(),
                              g_statistics->GetLastFrameTime() * 1000.0f);

  SDL_GL_SwapWindow(s_window);

  g_statistics->EndFrame();
}

void CaptureMouse()
{
  if (s_mouse_captured)
    return;

  SDL_CaptureMouse(SDL_TRUE);
  SDL_SetRelativeMouseMode(SDL_TRUE);
  std::fprintf(stdout, "Mouse captured\n");
  s_mouse_captured = true;
}

void ReleaseMouse()
{
  if (!s_mouse_captured)
    return;

  SDL_CaptureMouse(SDL_FALSE);
  SDL_SetRelativeMouseMode(SDL_FALSE);
  std::fprintf(stdout, "Mouse released\n");
  s_mouse_captured = false;
}

void MainLoop()
{
  SDL_PumpEvents();
  for (;;)
  {
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
      if (ev.type == SDL_QUIT)
      {
        return;
      }
      else if (ev.type == SDL_WINDOWEVENT)
      {
        switch (ev.window.event)
        {
          case SDL_WINDOWEVENT_FOCUS_GAINED:
            CaptureMouse();
            break;

          case SDL_WINDOWEVENT_FOCUS_LOST:
            ReleaseMouse();
            break;
        }
      }
      else if (ev.type == SDL_MOUSEMOTION)
      {
        if (ev.motion.xrel != 0)
          s_camera.ModYaw(float(-ev.motion.xrel));
        if (ev.motion.yrel != 0)
          s_camera.ModPitch(float(-ev.motion.yrel));
      }
      else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP)
      {
        const bool down = (ev.type == SDL_KEYDOWN);
        const float value = (down) ? 1.0f : -1.0f;
        switch (ev.key.keysym.scancode)
        {
          case SDL_SCANCODE_W:
            s_camera.ModViewVector(glm::vec3(0.0f, value, 0.0f));
            break;
          case SDL_SCANCODE_S:
            s_camera.ModViewVector(glm::vec3(0.0f, -value, 0.0f));
            break;
          case SDL_SCANCODE_D:
            s_camera.ModViewVector(glm::vec3(value, 0.0f, 0.0f));
            break;
          case SDL_SCANCODE_A:
            s_camera.ModViewVector(glm::vec3(-value, 0.0f, 0.0f));
            break;
          case SDL_SCANCODE_LSHIFT:
            s_camera.SetTurboEnabled(down);
            break;
        }
      }
    }

    Render();
  }
}
} // namespace

int main(int argc, char* argv[])
{
  if (argc < 2)
    return EXIT_FAILURE;

  {
    auto fp = Util::FOpenUniquePtr(argv[1], "rb");
    if (!fp)
      return EXIT_FAILURE;

    s_bsp = BSP::Load(fp.get());
    if (!s_bsp)
      return EXIT_FAILURE;
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return EXIT_FAILURE;

  s_window = SDL_CreateWindow("q3mv", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
  if (!s_window)
    return EXIT_FAILURE;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_CORE | SDL_GL_CONTEXT_DEBUG_FLAG);

  SDL_GLContext ctx = SDL_GL_CreateContext(s_window);
  if (!ctx || SDL_GL_MakeCurrent(s_window, ctx) != 0)
    return EXIT_FAILURE;

  SDL_GL_SetSwapInterval(0);

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
    return EXIT_FAILURE;

  if (!Setup())
  {
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(s_window);
    return EXIT_FAILURE;
  }

  MainLoop();

  s_bsp_renderer.reset();
  s_bsp.reset();

  g_resource_manager->UnloadAllResources();

  SDL_GL_MakeCurrent(nullptr, nullptr);
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(s_window);

  return EXIT_SUCCESS;
}
