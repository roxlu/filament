cmake_minimum_required(VERSION 3.15)
project(filgl C CXX)

set(bd ${CMAKE_CURRENT_LIST_DIR})  # our base directory
set(id ${bd}/roxlu-installed)      # glfw and possible other deps. 
set(sd ${bd}/src)                  # source directory for the test
set(fd ${bd}/../roxlu-installed)   # filament install directory.
set(ld ${fd}/lib/x86_64/)          # filament lib dir
set(fb ${bd}/../roxlu-build)       # filament build dir

include_directories(
  ${fd}/include
  ${id}/include
  )


list(APPEND roxlu_libs

  ${ld}/filament.lib

  # Here we use the lib files which get compiled when we
  # make a change to the `backend` sources. This allows us
  # to develop and don't recompile and reinstall the full
  # filament library ... which takes a bit too long. 
  ${fb}/libs/math/Release/math.lib
  ${fb}/libs/utils/Release/utils.lib
  ${fb}/filament/backend/Release/backend.lib
  ${fb}/libs/bluegl/Release/bluegl.lib

  ${ld}/filabridge.lib
  ${ld}/filaflat.lib
  ${ld}/filamat.lib
  ${ld}/filamat_lite.lib
  ${ld}/filameshio.lib
  ${ld}/geometry.lib
  ${ld}/gltfio.lib
  ${ld}/gltfio_core.lib
  ${ld}/ibl.lib
  ${ld}/image.lib
  ${ld}/matdbg.lib
  ${ld}/meshoptimizer.lib
  ${ld}/rays.lib
  ${ld}/shaders.lib
  ${ld}/smol-v.lib
  ${id}/lib/glfw3.lib
  Opengl32.lib
  ws2_32.lib
  psapi.lib
  iphlpapi.lib
  gdi32.lib
  )

set(test_name test-shared-gl-context)
add_executable(${test_name} ${sd}/test-shared-gl-context.cpp)
set_property(TARGET ${test_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
set_property(TARGET ${test_name} APPEND_STRING PROPERTY LINK_FLAGS "/force:multiple") # fixes duplicate symbols issue (bluegl<>opengl32), https://github.com/google/filament/issues/652
target_link_libraries(${test_name} ${roxlu_libs})
install(TARGETS ${test_name} DESTINATION bin)




