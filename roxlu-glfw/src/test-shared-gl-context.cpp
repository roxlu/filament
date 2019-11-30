
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filament/Engine.h>
//#include <filament/LightManager.h>
//#include <filament/RenderableManager.h>
#include <filament/Scene.h>
//#include <filament/TransformManager.h>
#include <filament/View.h>
#include <filament/Renderer.h>

/* -------------------------------------------- */

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

/* -------------------------------------------- */

int win_h = 1024;
int win_w = 768;

/* -------------------------------------------- */

#define USE_GL 0

/* -------------------------------------------- */
  
int main(int argc, char* argv[]) {

  printf("Testing Filament with GLFW and a Shared Context\n");
  
  glfwSetErrorCallback(error_callback);

  if(!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_DECORATED, GL_TRUE);

#if 0 == USE_GL
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif 
 
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

  HWND win_hwnd = glfwGetWin32Window(win);

#if 0 != USE_GL
  /* Get the pixel format that has been selected by GLFW */
  HDC win_hdc = GetDC(win_hwnd);
  if (nullptr == win_hdc) {
    printf("Failed to get the HDC from our window. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  int win_pix_fmt = GetPixelFormat(win_hdc);
  if (0 == win_pix_fmt) {
    printf("Failed to get the pixel format that was selected through GLFW. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  printf("- win_pix_fmt: %d.\n", win_pix_fmt);
#endif
  
  filament::Engine* engine = filament::Engine::create();
  if (nullptr == engine) {
    printf("Failed to create the filament::Engine. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::SwapChain* swap = engine->createSwapChain(win_hwnd);
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
  
  /* 
    Engine* engine = Engine::create();
    SwapChain* swapChain = engine->createSwapChain(nativeWindow);
    Renderer* renderer = engine->createRenderer();
   */
  
  /* @todo load GL functions. */

  /* -------------------------------------------------------------------- */

  /* Our main render loop. */
  while(!glfwWindowShouldClose(win)) {
    
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*
    glViewport(0, 0, win_w, win_h);
    glClearColor(0.13f, 0.13f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    */
    if (ren->beginFrame(swap)) {
      // for each View
      ren->render(view);
      ren->endFrame();
    }

#if 1 == USE_GL
    glfwSwapBuffers(win);
#endif
    
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
