//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <mutex>
#include <thread>
#include <sstream>

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Renderer.h>
#include <backend/DriverEnums.h> /* Used for `BackendConfig` */

#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/glext.h>
#include <gl/wglext.h>

/* ----------------------------------------------------------- */

class GlContext {
public:
  void print(const char* name);

public:
  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
  PIXELFORMATDESCRIPTOR fmt = {};
  GlContext* shared = nullptr;
  HWND hwnd = nullptr;
  HGLRC gl = nullptr;
  HDC dc = nullptr;
  int dx = -1;
};

static int create_tmp_context(GlContext& ctx);
static int destroy_tmp_context(GlContext& ctx);
static int create_main_context(GlContext& tmp, GlContext& main);
static int destroy_main_context(GlContext& main);
/* -------------------------------------------- */

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

/* -------------------------------------------- */

uint32_t win_w = 1024;
uint32_t win_h = 768;

/* -------------------------------------------- */

int main(int argc, char* argv[]) {

  printf("Testing Filament with GLFW and a Shared Context\n");
  
  glfwSetErrorCallback(error_callback);

  if(!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_DECORATED, GL_TRUE);

  GLFWwindow* win = glfwCreateWindow(win_w, win_h, "Filament + GLFW: shared OpenGL Context.", NULL, NULL);
  if(!win) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(win, resize_callback);
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);
  glfwSetCursorPosCallback(win, cursor_callback);
  glfwSetMouseButtonCallback(win, button_callback);
  glfwSetScrollCallback(win, scroll_callback);
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  if (0 == gladLoadGL()) {
    printf("Failed to load glad ... (exiting).\n");
    exit(EXIT_FAILURE);
  }
  printf("! glfw is using GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
  printf("! glfw is using GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));

  std::ostringstream oss;
  oss << std::this_thread::get_id();
  std::string thread_id = oss.str();
  printf("! glfw thread id: %s\n", thread_id.c_str());
  
  HWND win_hwnd = glfwGetWin32Window(win);
  printf("! glfw hwnd: %p\n", win_hwnd);

  GlContext tmp_ctx;
  if (0 != create_tmp_context(tmp_ctx)) {
    printf("Failed to create tmp context. (exiting)\n");
    exit(EXIT_FAILURE);
  }
  
  GlContext fila_ctx;
  if (0 != create_main_context(tmp_ctx, fila_ctx)) {
    printf("Failed to create main context for filament. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::backend::BackendConfig backend_cfg = {};
  backend_cfg.context = fila_ctx.gl;
  backend_cfg.hdc = fila_ctx.dc;
  
  filament::Engine* engine = filament::Engine::create(filament::backend::Backend::OPENGL, nullptr, &backend_cfg);
  if (nullptr == engine) {
    printf("Failed to create the filament::Engine. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::SwapChain* swap = engine->createSwapChain(fila_ctx.hwnd);
  if (nullptr == swap) {
    printf("Failed to create the filament::SwapChain. (exiting).\n");
    exit(EXIT_FAILURE);
  }
  
  filament::Renderer* ren = engine->createRenderer();
  if (nullptr == ren) {
    printf("Failed to create the filament::Renderer. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::Camera* cam = engine->createCamera();
  if (nullptr == cam) {
    printf("Failed to create the filament::Camera. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::View* view = engine->createView();
  if (nullptr == view) {
    printf("Failed to create the filament::View. (exiting).\n");
    exit(EXIT_FAILURE);
  }
  
  filament::Scene* scene = engine->createScene();
  if (nullptr == scene) {
    printf("Failed to create the filament::Scene. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  view->setCamera(cam);
  view->setScene(scene);
  view->setViewport({0, (int32_t)(win_h * 0.5), win_w, (uint32_t)(win_h * 0.5)});
  view->setClearColor({1.0f, 1.0f, 0.0f, 1.0f});

  printf("! glfw ready.\n");
  
  /* @todo load GL functions. */
  //  return 0;
  /* -------------------------------------------------------------------- */

  /* Our main render loop. */
  while(!glfwWindowShouldClose(win)) {

    glfwMakeContextCurrent(win);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w * 0.5, win_h * 0.5);
    glClearColor(1.13f, 0.13f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*
    if (ren->beginFrame(swap)) {
      ren->render(view);
      ren->endFrame();
    }
    */
    
    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  glfwTerminate();

  return EXIT_SUCCESS;
}

void char_callback(GLFWwindow* win, unsigned int key) {
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {

  if (GLFW_RELEASE == action) {
    return;
  }
  
  switch(key) {
    case GLFW_KEY_ESCAPE: {
      glfwSetWindowShouldClose(win, GL_TRUE);
      break;
    }
  };
}

void resize_callback(GLFWwindow* window, int width, int height) {
}

void cursor_callback(GLFWwindow* win, double x, double y) {
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
}

void button_callback(GLFWwindow* win, int bt, int action, int mods) {
}

void error_callback(int err, const char* desc) {
  printf("GLFW error: %s (%d)\n", desc, err);
}

/* -------------------------------------------------------------------- */

static int create_tmp_context(GlContext& ctx) {

  int r = 0;
  
  /* Step 1: create a tmp window. */
  ctx.hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
  if (nullptr == ctx.hwnd) {
    printf("Failed to create our tmp window.\n");
    r = -1;
    goto error;
  }

  /* Step 2: set the pixel format. */
  ctx.fmt.nSize = sizeof(ctx.fmt);
  ctx.fmt.nVersion = 1;
  ctx.fmt.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  ctx.fmt.iPixelType = PFD_TYPE_RGBA;
  ctx.fmt.cColorBits = 32;
  ctx.fmt.cAlphaBits = 8;
  ctx.fmt.cDepthBits = 24;

  ctx.dc = GetDC(ctx.hwnd);
  if (nullptr == ctx.dc) {
    printf("Failed to get the HDC from our tmp window.\n");
    r = -2;
    goto error;
  }

  ctx.dx = ChoosePixelFormat(ctx.dc, &ctx.fmt);
  if (0 == ctx.dx) {
    printf("Failed to find a pixel format for our tmp hdc.\n");
    r = -3;
    goto error;
  }

  if (FALSE == SetPixelFormat(ctx.dc, ctx.dx, &ctx.fmt)) {
    printf("Failed to set the pixel format on our tmp hdc.\n");
    r = -4;
    goto error;
  }

  /* Step 3. Create temporary GL context. */
  ctx.gl = wglCreateContext(ctx.dc);
  if (nullptr == ctx.gl) {
    printf("Failed to create out temporary GL context.\n");
    r = -5;
    goto error;
  }

  if (FALSE == wglMakeCurrent(ctx.dc, ctx.gl)) {
    printf("Failed to make our temporary GL context current.\n");
    r = -6;
    goto error;
  }

  /* Step 4. Get the extension we need for a more feature-rich context. */
  ctx.wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
  if (nullptr == ctx.wglChoosePixelFormatARB) {
    printf("Failed to get the `wglChoosePixelFormatARB()` function.\n");
    r = -7;
    goto error;
  }
  
  ctx.wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
  if (nullptr == ctx.wglCreateContextAttribsARB) {
    printf("wglGetProcAddress() failed.\n");
    r = -8;
    goto error;
  }

 error:

  if (r < 0) {
    if (0 != destroy_tmp_context(ctx)) {
      printf("After failing to create a tmp context ... we also failed to deallocate some temporaries :(\n");
    }
  }

  return r;
}

static int destroy_tmp_context(GlContext& ctx) {

  int r = 0;
  
  if (nullptr != ctx.hwnd) {
    if (FALSE == DestroyWindow(ctx.hwnd)) {
      printf("Failed to destroy the hwnd.\n");
      r -= 1;
    }
  }

  if (nullptr != ctx.gl) {
    
    if (FALSE == wglMakeCurrent(nullptr, nullptr)) {
      printf("Failed to reset any current OpenGL contexts.\n");
    }
    
    if (FALSE == wglDeleteContext(ctx.gl)) {
      printf("Failed to delete the temporary context.\n");
      r -= 2;
    }
  }

  /* Cleanup the members. */
  ctx.hwnd = nullptr;
  ctx.dc = nullptr;
  ctx.dx = -1;
  ctx.gl = nullptr;
  ctx.shared = nullptr;
  ctx.wglChoosePixelFormatARB = nullptr;
  ctx.wglCreateContextAttribsARB = nullptr;

  memset((char*)&ctx.fmt, 0x00, sizeof(ctx.fmt));
  
  return r;
}

static int create_main_context(GlContext& tmp, GlContext& main) {

  int r = 0;
  UINT fmt_count = 0;
  HGLRC shared_gl = nullptr;

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

  /* Validate */
  if (nullptr == tmp.gl) {
    printf("Given tmp context is invalid.\n");
    r = -1;
    goto error;
  }

  if (nullptr == tmp.wglChoosePixelFormatARB) {
    printf("Given tmp context has no `wglChoosePixelFormatARB()` set.\n");
    r = -2;
    goto error;
  }

  if (nullptr == tmp.wglCreateContextAttribsARB) {
    printf("Given tmp context has no `wglCreateContextAttribsARB()` set.\n");
    r = -3;
    goto error;
  }
  
  /* Step 1: we still need a HWND for our main context. */
  main.hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
  if (nullptr == main.hwnd) {
    printf("Failed to create main window (hwnd).\n");
    r = -4;
    goto error;
  }

  /* Step 2: set the pixel format */
  main.dc = GetDC(main.hwnd);
  if (nullptr == main.dc) {
    printf("Failed to get the HDC from our main window.\n");
    r = -5;
    goto error;
  }

  /* Find the best matching pixel format index. */
  if (FALSE == tmp.wglChoosePixelFormatARB(main.dc, pix_attribs, NULL, 1, &main.dx, &fmt_count)) {
    printf("Failed to choose a valid pixel format for our main hdc.\n");
    r = -6;
    goto error;
  }

  /* Now that we have found the index, fill our format descriptor. */
  if (0 == DescribePixelFormat(main.dc, main.dx, sizeof(main.fmt), &main.fmt)) {
    printf("Failed to fill our main pixel format descriptor.\n");
    r = -7;
    goto error;
  }

  if (FALSE == SetPixelFormat(main.dc, main.dx, &main.fmt)) {
    printf("Failed to set the pixel format on our main dc.\n");
    r = -8;
    goto error;
  }

  /* Step 3: create our main context. */
  if (nullptr != main.shared) {
    shared_gl = main.shared->gl;
    printf("Using shared GL context: %p\n", shared_gl);
  }
  
  main.gl = tmp.wglCreateContextAttribsARB(main.dc, shared_gl, ctx_attribs);
  if (nullptr == main.gl) {
    printf("Failed to create our main OpenGL context.\n");
    r = -8;
    goto error;
  }
  
 error:
  
  if (r < 0) {
    printf("Failed to create the main context.\n");
    if (0 != destroy_main_context(main)) {
      printf("After failing to create our main context, we also couldn't clean it up correctly.\n");
    }
  }

  return r;
}

static int destroy_main_context(GlContext& main) {
  return destroy_tmp_context(main);
}

/* ------------------------------------------------------------- */

void GlContext::print(const char* name) {

  if (nullptr == name) {
    printf("Cannot print, pass in a name.\n");
    return;
  }
  
  if (nullptr == gl) {
    printf("Cannot print info, not initialized (gl == nullptr) .\n");
    return;
  }

  if (nullptr == dc) {
    printf("Cannot print into, not initialized (dc == nullptr). \n");
    return;
  }

  wglMakeCurrent(dc, gl);
  /*
  printf("%s: HGLRC: %p\n", name, gl);
  printf("%s: GL_VERSION: %s\n", name, glGetString(GL_VERSION));
  printf("%s: GL_VENDOR: %s\n", name, glGetString(GL_VENDOR));
  */
}

/* ------------------------------------------------------------- */
