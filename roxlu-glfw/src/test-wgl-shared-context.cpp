#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <OpenGlContext.h>

using namespace poly;

/* -------------------------------------------- */

int main(int argc, const char* argv[]) {

  printf("test-wgl-shared-context\n");
  
  OpenGlContextSettings cfg;
  cfg.name = "main";
  cfg.title = "gl-test";
  cfg.x = 0;
  cfg.y = 0;
  cfg.width = 1024;
  cfg.height = 768;
  cfg.is_floating = true;
  cfg.is_decorated = false;
  cfg.create_window = true;
  
  OpenGlContext ctx_main;
  if (0 != ctx_main.init(cfg)) {
    printf("Failed to init. (exiting)\n");
    exit(EXIT_FAILURE);
  }

  /* We have to make current and load functions with glad. */
  ctx_main.makeCurrent();
  if (0 == gladLoadGL()) {
    printf("Failed to load glad. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  OpenGlContext ctx_shared;
  cfg.name = "shared";
  cfg.create_window = false;
  cfg.shared_context = &ctx_main;

  if (0 != ctx_shared.init(cfg)) {
    printf("Failed to create shared context. (exiting).\n");
    exit(EXIT_FAILURE);
  }
  
  ctx_main.print();
  ctx_main.show();
  ctx_main.makeCurrent();
  
  float r = 0.0f;
  
  while (0 == ctx_main.processMessages()) {
    
    ctx_main.makeCurrent();
    
    glClearColor(r, 1.0f - r, 0.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    r += 0.01;
    ctx_main.swapBuffers();
  }
  
  ctx_main.shutdown();
  ctx_shared.shutdown();
  
  return EXIT_SUCCESS;
}


/* -------------------------------------------- */
