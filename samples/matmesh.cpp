/*

   -------------------------------------------------------- 

   Copyright (C) 2018 The Android Open Source Project
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
        http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   ---------------------------------------------------------------

                                                  oooo
                                                  '888
                   oooo d8b  .ooooo.  oooo    ooo  888  oooo  oooo
                   '888""8P d88' '88b  '88b..8P'   888  '888  '888
                    888     888   888    Y888'     888   888   888
                    888     888   888  .o8"'88b    888   888   888
                   d888b    'Y8bod8P' o88'   888o o888o  'V88V"V8P'
   
                                                     www.roxlu.com
                                             www.twitter.com/roxlu
   
   ---------------------------------------------------------------


   MATERIAL AND MESH TESTER
   =========================

   I created this sample which is based on the `hellopbr` sample to 
   experiment filament materials and meshes. You create a self
   contained material (no custom uniforms will be passed/set) and
   and a filamesh.

   USAGE:

     matmesh --material [path-to-compiled-filamat] --mesh [path-to-filamesh].

 */

#include <getopt/getopt.h>
#include <fstream>
#include <filament/Engine.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/Renderer.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/TransformManager.h>
#include <filament/View.h>
#include <utils/EntityManager.h>
#include <filameshio/MeshReader.h>
#include <filamentapp/Config.h>
#include <filamentapp/FilamentApp.h>
#include "generated/resources/resources.h"
#include "generated/resources/monkey.h"

using namespace filament;
using namespace filamesh;
using namespace filament::math;

using Backend = Engine::Backend;

struct App {
  utils::Entity light;
  Material* material;
  MaterialInstance* materialInstance;
  MeshReader::Mesh mesh;
  mat4f transform;
};

static const char* IBL_FOLDER = "venetian_crossroads_2k";

/* -------------------------------------------------------- */

class MatMeshOptions {
public:
  bool isValid();
  
public:
  std::string material_filepath;
  std::string mesh_filepath;
};

/* -------------------------------------------------------- */

static void pre_render(
  filament::Engine* engine,
  filament::View* view,
  filament::Scene* scene,
  filament::Renderer* renderer
);

static int handle_command_line_arguments(int argc, char* argv[], MatMeshOptions& result);
static int load_file(const std::string& filepath, std::string& result);

/* -------------------------------------------------------- */

int main(int argc, char** argv) {
  
  Config config;
  config.title = "matmesh";
  config.iblDirectory = FilamentApp::getRootAssetsPath() + IBL_FOLDER;

  MatMeshOptions matmesh_options;
  if (0 != handle_command_line_arguments(argc, argv, matmesh_options)) {
    printf("Failed to parse command line arguments. (exiting).\n");
    exit(EXIT_FAILURE);
  }

  if (false == matmesh_options.isValid()) {
    printf("Invalid options (exiting).\n");
    exit(EXIT_FAILURE);
  }

  App app;
  auto setup = [config, &app, matmesh_options](Engine* engine, View* view, Scene* scene) {
    auto& tcm = engine->getTransformManager();
    auto& rcm = engine->getRenderableManager();
    auto& em = utils::EntityManager::get();

    /* Load material file */
    std::string mat_data;
    if (0 != load_file(matmesh_options.material_filepath, mat_data)) {
      printf("Failed to load `%s` (exiting). \n", matmesh_options.material_filepath.c_str());
      exit(EXIT_FAILURE);
    }

    printf("Loaded `%s` with `%zu` bytes.\n", matmesh_options.material_filepath.c_str(), mat_data.size());

    /* Enable bloom */
    filament::View::BloomOptions bloom_opt;
    bloom_opt.strength = 1.0f;
    bloom_opt.enabled = true;
    bloom_opt.levels = 12;
    view->setBloomOptions(bloom_opt);
    view->setPostProcessingEnabled(true); 
    
    /* Instantiate material. */
    app.material = Material::Builder()
      .package(mat_data.data(), mat_data.size())
      .build(*engine);

    /* Disable skybox. */
    scene->setSkybox(nullptr);
    
    auto mi = app.materialInstance = app.material->createInstance();
   
    /* Add geometry into the scene. */
    std::string mesh_data;
    if (0 != load_file(matmesh_options.mesh_filepath, mesh_data)) {
      printf("Failed to load the mesh file `%s` (exiting). \n", matmesh_options.mesh_filepath.c_str());
      exit(EXIT_FAILURE);
    }
      
    app.mesh = MeshReader::loadMeshFromBuffer(engine, mesh_data.data(), nullptr, nullptr, mi);
    auto ti = tcm.getInstance(app.mesh.renderable);
    app.transform = mat4f{ mat3f(1), float3(0, 0, -4) } * tcm.getWorldTransform(ti);
    rcm.setCastShadows(rcm.getInstance(app.mesh.renderable), false);
    scene->addEntity(app.mesh.renderable);

    /* Add light sources into the scene. */
    app.light = em.create();
    LightManager::Builder(LightManager::Type::SUN)
      .color(Color::toLinear<ACCURATE>(sRGBColor(0.98f, 0.92f, 0.89f)))
      .intensity(110000)
      .direction({ 0.7, -1, -0.8 })
      .sunAngularRadius(1.9f)
      .castShadows(false)
      .build(*engine, app.light);
    scene->addEntity(app.light);
  };

  auto cleanup = [&app](Engine* engine, View*, Scene*) {
    engine->destroy(app.light);
    engine->destroy(app.materialInstance);
    engine->destroy(app.mesh.renderable);
    engine->destroy(app.material);
  };


  FilamentApp::get().animate([&app](Engine* engine, View* view, double now) {
    auto& tcm = engine->getTransformManager();
    auto ti = tcm.getInstance(app.mesh.renderable);
    tcm.setTransform(ti, app.transform * mat4f::rotation(now * 0.1, float3{ 0, 1, 0 }));
  });


  FilamentApp::get().run(config, setup, cleanup, nullptr, pre_render);

  return 0;
}

/* -------------------------------------------------------- */

static void pre_render(
  filament::Engine* engine,
  filament::View* view,
  filament::Scene* scene,
  filament::Renderer* renderer
)
{
  if (nullptr == renderer) {
    printf("`renderer` is nullptr.\n");
    return;
  }

  renderer->setClearOptions({
    .clearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
    .clear = true
  });
}
  
/* -------------------------------------------------------- */


/*
  --material [filepath]:  Path to the material file to load. 
  --mesh [filepath]: Path to the filamesh to load.
 */
static int handle_command_line_arguments(int argc, char* argv[], MatMeshOptions& result) {
  
  const char* opt_str = "m";
  struct option opts[] = {
    { "material", required_argument, nullptr, 'm' },
    { "mesh", required_argument, nullptr, 'o' },
    { nullptr, 0, nullptr, 0 }
  };

  int opt;
  int opt_dx = 0;

  while ((opt = getopt_long(argc, argv, opt_str, opts, &opt_dx)) >= 0) {
      
    std::string arg(optarg ? optarg : "");
      
    switch (opt) {
        
      case 'm': {
        result.material_filepath = arg;
        break;
      }

      case 'o': {
        result.mesh_filepath = arg;
        break;
      }
          
      default: {
        break;
      }
    }
  }

  return 0;
}

/* -------------------------------------------------------- */

static int load_file(const std::string& filepath, std::string& result) {

  if (0 == filepath.size()) {
    printf("Cannot load from file, given filepath is empty.\n");
    return -1;
  }
  
  std::ifstream ifs(filepath.c_str(), std::ios::in | std::ios::binary);
  if (false == ifs.is_open()) {
    printf("Failed to load `%s`.\n", filepath.c_str());
    return -2;
  }
  
  result = std::string((std::istreambuf_iterator<char>(ifs)) , std::istreambuf_iterator<char>());

  return 0;
}

/* -------------------------------------------------------- */

bool MatMeshOptions::isValid() {

  if (0 == material_filepath.size()) {
    printf("`material_filepath` not set.\n");
    return false;
  }

  if (0 == mesh_filepath.size()) {
    printf("`mesh_filepath` not set.\n");
    return false;
  }

  return true;
}

/* -------------------------------------------------------- */
