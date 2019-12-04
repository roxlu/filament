#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <OpenGlContext.h>

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Renderer.h>
#include <backend/DriverEnums.h> /* Used for `BackendConfig` */

using namespace poly;

/* -------------------------------------------- */

int main(int argc, const char* argv[]) {

  OpenGlContextSettings cfg;
  cfg.name = "main";
  cfg.title = "test with filament and my own GL context";
  cfg.x = 0;
  cfg.y = 0;
  cfg.width = 1024;
  cfg.height = 768;
  cfg.is_floating = false;
  cfg.is_decorated = true;
  cfg.create_window = true;

  /* Create our own context */
  OpenGlContext ctx_main;
  if (0 != ctx_main.init(cfg)) {
    printf("Failed to init. (exiting)\n");
    exit(EXIT_FAILURE);
  }

  /* At this point we have to load the GL functions. */
  ctx_main.makeCurrent();
  if (0 == gladLoadGL()) {
    printf("Failed to load glad. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  /* Create context for filament. */
  OpenGlContext ctx_fila;
  cfg.name = "shared";
  cfg.title = "fila window";
  cfg.x = cfg.width;
  cfg.height = cfg.height * 0.5;
  cfg.create_window = true;
  cfg.shared_context = &ctx_main;

  if (0 != ctx_fila.init(cfg)) {
    printf("Failed to create shared context. (exiting).\n");
    exit(EXIT_FAILURE);
  }
  ctx_fila.show();

  // --
  filament::backend::BackendConfig backend_cfg = {};
  backend_cfg.context = ctx_fila.gl;
  backend_cfg.hdc = ctx_fila.hdc;
  
  filament::Engine* engine = filament::Engine::create(filament::backend::Backend::OPENGL, nullptr, &backend_cfg);
  if (nullptr == engine) {
    printf("Failed to create the filament::Engine. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  filament::SwapChain* swap = engine->createSwapChain(ctx_fila.hwnd);
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
  view->setViewport({0, 0, cfg.width, cfg.height}); 
  view->setClearColor({1.0f, 1.0f, 0.0f, 1.0f});

  ctx_main.print();
  ctx_main.show();
  ctx_main.makeCurrent();
  
  float r = 0.0f;
  
  while (0 == ctx_main.processMessages()) {

#if 1    
    ctx_main.makeCurrent();
    glClearColor(r, 1.0f - r, 0.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
#endif

    if (ren->beginFrame(swap)) {
      ren->render(view);
      ren->endFrame();
    }

    r += 0.01;
    ctx_main.swapBuffers();
  }
  
  printf("Shutting down ...\n");
  ctx_fila.shutdown();
  ctx_main.shutdown();
  
  return EXIT_SUCCESS;
}


/* -------------------------------------------- */
