#ifndef POLY_OPENGL_CONTEXT_H
#define POLY_OPENGL_CONTEXT_H

#define WIN32_LEAN_AND_MEAN
#include <string>
#include <windows.h>
#include <glad/glad.h>
#include <gl/glext.h>
#include <gl/wglext.h>

namespace poly {

  /* -------------------------------------------- */
  
  class OpenGlContext;

  /* -------------------------------------------- */
  
  class OpenGlContextSettings {
  public:
    bool isValid();
  
  public:
    OpenGlContext* shared_context = nullptr;
    uint32_t x = UINT32_MAX;
    uint32_t y = UINT32_MAX;
    uint32_t height = UINT32_MAX;
    uint32_t width = UINT32_MAX;
    bool create_window = true; /* When set to `false` we will only create a GL context. */
    bool is_decorated = false; /* Draw minimize, maximize, window title etc. */
    bool is_resizable = false; /* Can we resize the window; you most likely want to put this to false when `is_decorated` is false too. */
    bool is_floating = false; /* Top most window? */
    std::string name;
    std::string title;
  };

  /* -------------------------------------------- */

  class OpenGlContext {
  public:
    OpenGlContext() = default;
    ~OpenGlContext();
    int init(OpenGlContextSettings& cfg);
    int shutdown();
    int show();
    int print();

    int processMessages();
    int makeCurrent();
    int swapBuffers();

  private:
    int getWindowStyle(DWORD& result);
    int getWindowStyleEx(DWORD& result);

  public:
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    PIXELFORMATDESCRIPTOR fmt = {};
    OpenGlContextSettings settings;
    HWND hwnd = nullptr;
    HGLRC gl = nullptr;
    HDC hdc = nullptr;
    int dx = -1;
    MSG msg = {};
  };

  /* -------------------------------------------- */

} /* namespace poly */

#endif
