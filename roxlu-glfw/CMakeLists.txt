cmake_minimum_required(VERSION 3.15)
project(filgl C CXX)

set(bd ${CMAKE_CURRENT_LIST_DIR})  # our base directory
set(id ${bd}/roxlu-installed)      # glfw and possible other deps. 
set(sd ${bd}/src)                  # source directory for the test
set(fd ${bd}/../roxlu-installed)   # filament install directory.
set(ld ${fd}/lib/x86_64/)          # filament lib dir

include_directories(
  ${fd}/include
  ${id}/include
  )

list(APPEND roxlu_libs
  Opengl32.lib
  ${ld}/backend.lib
  ${ld}/bluegl.lib
  ${ld}/filabridge.lib
  ${ld}/filaflat.lib
  ${ld}/filamat.lib
  ${ld}/filamat_lite.lib
  ${ld}/filament.lib
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
  ${ld}/utils.lib
  ${id}/lib/glfw3.lib

  ws2_32.lib
  psapi.lib
  iphlpapi.lib
  )

set(test_name test-shared-gl-context)
add_executable(${test_name} ${sd}/test-shared-gl-context.cpp)
set_property(TARGET ${test_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_link_libraries(${test_name} ${roxlu_libs})
install(TARGETS ${test_name} DESTINATION bin)



