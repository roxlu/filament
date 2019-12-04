#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* -------------------------------------------- */

#define WIN32_LEAN_AND_MEAN
#include <string>
#include <windows.h>
#include <glad/glad.h>
#include <gl/glext.h>
#include <gl/wglext.h>

class OpenGlContextSettings {
public:
  bool isValid();
  
public:
  std::string name;
  uint32_t width = UINT32_MAX;
  uint32_t height = UINT32_MAX;
};

/* -------------------------------------------- */

class OpenGlContext {
public:
  int init(OpenGlContextSettings& cfg);
  int shutdown();
  void print();

public:
  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
  PIXELFORMATDESCRIPTOR fmt = {};
  OpenGlContextSettings settings;
  HWND hwnd = nullptr;
  HGLRC gl = nullptr;
  HDC hdc = nullptr;
  int dx = -1;
};

/* -------------------------------------------- */

int main(int argc, const char* argv[]) {

  printf("test-wgl-shared-context\n");
  
  OpenGlContextSettings cfg;
  cfg.name = "main";
  
  OpenGlContext ctx;

  if (0 != ctx.init(cfg)) {
    printf("Failed to init. (exiting)\n");
    exit(EXIT_FAILURE);
  }

  ctx.print();
  
  return EXIT_SUCCESS;
}

/* -------------------------------------------- */

bool OpenGlContextSettings::isValid() {

  if (0 == name.size()) {
    printf("Error: name is not set. We want our `OpenGlContext` to have a name.\n");
    return false;
  }
  
  return true;
}

/* -------------------------------------------- */

int OpenGlContext::init(OpenGlContextSettings& cfg) {

  PIXELFORMATDESCRIPTOR tmp_fmt = {};
  HWND tmp_hwnd = nullptr;
  HGLRC tmp_gl = nullptr;
  HDC tmp_hdc = nullptr;
  int tmp_dx = -1;
  int r = 0;
  UINT fmt_count = 0;
  
  /* The pixel format attributes that we need. */
  const int pix_attribs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_ALPHA_BITS_ARB, 8,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
    WGL_SAMPLES_ARB, 4,
    0
  };

  /* The OpenGL Rendering Context attributes we need. */
  int ctx_attribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 1,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0
  };
  
  if (false == cfg.isValid()) {
    printf("Given `OpenGlContextSettings` are invalid.\n");
    r = -1;
    goto error;
  }

  settings = cfg;

  if (nullptr != hwnd) {
    printf("Already initialized. Probably not what you want.\n");
    r = -2;
    goto error;
  }

    /* Step 1: create a tmp window. */
  tmp_hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
  if (nullptr == tmp_hwnd) {
    printf("Failed to create our tmp window.\n");
    r = -1;
    goto error;
  }

  /* Step 2: set the pixel format. */
  tmp_fmt.nSize = sizeof(tmp_fmt);
  tmp_fmt.nVersion = 1;
  tmp_fmt.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  tmp_fmt.iPixelType = PFD_TYPE_RGBA;
  tmp_fmt.cColorBits = 32;
  tmp_fmt.cAlphaBits = 8;
  tmp_fmt.cDepthBits = 24;

  tmp_hdc = GetDC(tmp_hwnd);
  if (nullptr == tmp_hdc) {
    printf("Failed to get the HDC from our tmp window.\n");
    r = -2;
    goto error;
  }

  tmp_dx = ChoosePixelFormat(tmp_hdc, &tmp_fmt);
  if (0 == tmp_dx) {
    printf("Failed to find a pixel format for our tmp hdc.\n");
    r = -3;
    goto error;
  }

  if (FALSE == SetPixelFormat(tmp_hdc, tmp_dx, &tmp_fmt)) {
    printf("Failed to set the pixel format on our tmp hdc.\n");
    r = -4;
    goto error;
  }

  /* Step 3. Create temporary GL context. */
  tmp_gl = wglCreateContext(tmp_hdc);
  if (nullptr == tmp_gl) {
    printf("Failed to create out temporary GL context.\n");
    r = -5;
    goto error;
  }

  if (FALSE == wglMakeCurrent(tmp_hdc, tmp_gl)) {
    printf("Failed to make our temporary GL context current.\n");
    r = -6;
    goto error;
  }

  /* Step 4. Get the extension we need for a more feature-rich context. */
  wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
  if (nullptr == wglChoosePixelFormatARB) {
    printf("Failed to get the `wglChoosePixelFormatARB()` function.\n");
    r = -7;
    goto error;
  }
  
  wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
  if (nullptr == wglCreateContextAttribsARB) {
    printf("wglGetProcAddress() failed.\n");
    r = -8;
    goto error;
  }
  
  /* ++++++++++++++++++ */

  /* @todo -> this is where we should create our real window! */
  
  /* Step 5: create the HWND for our main context. */
  hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
  if (nullptr == hwnd) {
    printf("Failed to create main window (hwnd).\n");
    r = -4;
    goto error;
  }

  /* Step 6: set the pixel format */
  hdc = GetDC(hwnd);
  if (nullptr == hdc) {
    printf("Failed to get the HDC from our main window.\n");
    r = -5;
    goto error;
  }

  /* Find the best matching pixel format index. */
  if (FALSE == wglChoosePixelFormatARB(hdc, pix_attribs, NULL, 1, &dx, &fmt_count)) {
    printf("Failed to choose a valid pixel format for our main hdc.\n");
    r = -6;
    goto error;
  }

  /* Now that we have found the index, fill our format descriptor. */
  if (0 == DescribePixelFormat(hdc, dx, sizeof(fmt), &fmt)) {
    printf("Failed to fill our main pixel format descriptor.\n");
    r = -7;
    goto error;
  }

  if (FALSE == SetPixelFormat(hdc, dx, &fmt)) {
    printf("Failed to set the pixel format on our main dc.\n");
    r = -8;
    goto error;
  }

  /* Step 7: finally, create our main context. */
  gl = wglCreateContextAttribsARB(hdc, nullptr, ctx_attribs);
  if (nullptr == gl) {
    printf("Failed to create our main OpenGL context.\n");
    r = -8;
    goto error;
  }

 error:
  
  if (r < 0) {
    shutdown();
  }

  return r;
}

int OpenGlContext::shutdown() {

  int r = 0;

  return r;
}

/* -------------------------------------------- */

void OpenGlContext::print() {

  if (0 == settings.name.size()) {
    printf("Cannot print, name not set. Did you initialize?\n");
    return;
  }

  if (nullptr == hdc) {
    printf("Cannot print, hdc is nullptr. Did you initialize?\n");
    return;
  }

  if (nullptr == gl) {
    printf("Cannot print, gl is nullptr. Did you initialize?\n");
    return;
  }
  
  if (FALSE == wglMakeCurrent(hdc, gl)) {
    printf("Cannot print. Failed to make the GL context current.\n");
    return;
  }
  
  printf("%s: HGLRC: %p\n", settings.name.c_str(), gl);
  printf("%s: GL_VERSION: %s\n", settings.name.c_str(), glGetString(GL_VERSION));
  printf("%s: GL_VENDOR: %s\n", settings.name.c_str(), glGetString(GL_VENDOR));
}



