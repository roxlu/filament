#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <OpenGlContext.h>

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Renderer.h>
#include <filament/RenderTarget.h>
#include <filament/Texture.h>
#include <backend/DriverEnums.h> /* Used for `BackendConfig` */

using namespace poly;

/* -------------------------------------------- */

static const std::string VS = R"(#version 430

  out vec2 v_tex;

  void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_tex.x = (x+1.0)*0.5;
    v_tex.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
  }

)";

static const std::string FS = R"(#version 430

  layout (location = 0) out vec4 o_fragcolor;
  layout (location = 0) uniform sampler2D u_tex;
  in vec2 v_tex;

  void main() {
     //vec4 col = texture(u_tex, v_tex);
     //o_fragcolor = col;
     o_fragcolor = vec4(0.4, 1.0, 0.3, 1.0);
  }

)";

                                
/* -------------------------------------------- */

static int create_shader(GLuint* out, GLenum type, const char* src);           /* create a shader, returns 0 on success, < 0 on error. e.g. create_shader(&vert, GL_VERTEX_SHADER, DRAWER_VS); */
static int create_program(GLuint* out, GLuint vert, GLuint frag, int link);    /* create a program from the given vertex and fragment shader. returns 0 on success, < 0 on error. e.g. create_program(&prog, vert, frag, 1); */
static int print_shader_compile_info(GLuint shader);                           /* prints the compile info of a shader. returns 0 when shader compiled, < 0 on error. */
static int print_program_link_info(GLuint prog);                               /* prints the program link info. returns 0 on success, < 0 on error. */

/* -------------------------------------------- */

int main(int argc, const char* argv[]) {

  OpenGlContextSettings cfg;
  cfg.name = "main";
  cfg.title = "test with filament and my own GL context + FBO";
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

  filament::Texture* tex_col = filament::Texture::Builder()
    .width(cfg.width)
    .height(cfg.height)
    .levels(1)
    .usage(filament::Texture::Usage::COLOR_ATTACHMENT | filament::Texture::Usage::SAMPLEABLE)
    .format(filament::Texture::InternalFormat::RGBA8)
    .build(*engine);

  printf("--- %u\n", tex_col->getId());

  view->setCamera(cam);
  view->setScene(scene);
  view->setViewport({0, 0, cfg.width, cfg.height}); 
  view->setClearColor({1.0f, 1.0f, 0.0f, 1.0f});

  ctx_main.print();
  ctx_main.show();
  ctx_main.makeCurrent();

  /* Create our output fullscreen shader. */
  GLuint fs_vert = 0;  
  if (0 != create_shader(&fs_vert, GL_VERTEX_SHADER, VS.c_str())) {
    printf("Failed to create the fullscreen vertex shader (exiting).\n");
    exit(EXIT_FAILURE);
  }

  GLuint fs_frag = 0;
  if (0 != create_shader(&fs_frag, GL_FRAGMENT_SHADER, FS.c_str())) {
    printf("Failed to create fullscreen fragment shader (exiting).\n");
    exit(EXIT_FAILURE);
  }
  
  GLuint fs_prog = 0;
  if (0 != create_program(&fs_prog, fs_vert, fs_frag, 1)) {
    printf("Failed to create the program. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  if (0 == vao) {
    printf("Failed to create our VAO. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  printf("fs_vert: %u\n", fs_vert);
  printf("fs_frag: %u\n", fs_frag);
  printf("fs_prog: %u\n", fs_prog);
  printf("vao: %u\n", vao);
  
  float r = 0.0f;
  
  while (0 == ctx_main.processMessages()) {

#if 1    
    ctx_main.makeCurrent();
    glClearColor(r, 1.0f - r, 0.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(fs_prog);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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

static int create_shader(GLuint* out, GLenum type, const char* src) {
 
  *out = glCreateShader(type);
  glShaderSource(*out, 1, &src, NULL);
  glCompileShader(*out);
  
  if (0 != print_shader_compile_info(*out)) {
    *out = 0;
    return -1;
  }
  
  return 0;
}

/* create a program, store the result in *out. when link == 1 we link too. returns -1 on error, otherwise 0 */
static int create_program(GLuint* out, GLuint vert, GLuint frag, int link) {
  *out = glCreateProgram();
  glAttachShader(*out, vert);
  glAttachShader(*out, frag);

  if(1 == link) {
    glLinkProgram(*out);
    if (0 != print_program_link_info(*out)) {
      *out = 0;
      return -1;
    }
  }

  return 0;
}

/* checks + prints program link info. returns 0 when linking didn't result in an error, on link erorr < 0 */
static int print_program_link_info(GLuint prog) {
  GLint status = 0;
  GLint count = 0;
  GLchar* error = NULL;
  GLsizei nchars = 0;

  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  if(status) {
    return 0;
  }

  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &count);
  if(count <= 0) {
    return 0;
  }

  error = (GLchar*)malloc(count);
  glGetProgramInfoLog(prog, count, &nchars, error);
  if (nchars <= 0) {
    free(error);
    error = NULL;
    return -1;
  }

  printf("\nPROGRAM LINK ERROR");
  printf("\n--------------------------------------------------------\n");
  printf("%s", error);
  printf("--------------------------------------------------------\n\n");

  free(error);
  error = NULL;
  return -1;
}

/* checks the compile info, if it didn't compile we return < 0, otherwise 0 */
static int print_shader_compile_info(GLuint shader) {

  GLint status = 0;
  GLint count = 0;
  GLchar* error = NULL;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status) {
    return 0;
  }
  
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &count);
  if (0 == count) {
    return 0;
  }

  error = (GLchar*) malloc(count);
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &count);
  if(count <= 0) {
    free(error);
    error = NULL;
    return 0;
  }

  glGetShaderInfoLog(shader, count, NULL, error);
  printf("\nSHADER COMPILE ERROR");
  printf("\n--------------------------------------------------------\n");
  printf("%s", error);
  printf("--------------------------------------------------------\n\n");

  free(error);
  error = NULL;
  return -1;
}
