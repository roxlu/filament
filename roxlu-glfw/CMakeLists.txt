cmake_minimum_required(VERSION 3.15)
project(filgl C CXX)
include(ExternalProject)

# Some dir aliases
# --------------------------------------------------
set(subsys "clang")                                              # clang or msvc.
set(list_dir ${CMAKE_CURRENT_LIST_DIR})                          # alias for the dir of this file.
set(prefix_dir ${CMAKE_INSTALL_PREFIX})                         # our install dir
set(base_dir ${list_dir}/roxlu-${subsys})                        # our base directory.
set(fila_install_dir ${list_dir}/../roxlu-${subsys}/installed)   # filament install directory.
set(fila_lib_dir ${fila_install_dir}/lib/x86_64/)                # filament lib dir.
set(fila_build_dir ${list_dir}/../roxlu-${subsys}/build/)        # filament build dir.
set(src_dir ${list_dir}/src)                                     # source directory for the test.
set(extern_dir ${list_dir}/extern)                               # external deps.

# GLFW
# --------------------------------------------------
ExternalProject_Add(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_SHALLOW 1
  UPDATE_COMMAND ""
  CMAKE_ARGS -DGLFW_BUILD_TESTS=Off -DGLFW_BUILD_DOCS=Off -DGLFW_BUILD_EXAMPLES=Off -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=lib -DUSE_MSVC_RUNTIME_LIBRARY_DLL=On
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/glfw3.lib
  )

ExternalProject_Get_Property(glfw install_dir)
set(glfw_install_dir ${install_dir})
include_directories(${glfw_install_dir}/include)
list(APPEND roxlu_deps glfw)

include_directories(
  ${fila_install_dir}/include
  ${prefix_dir}/include
  ${extern_dir}/glad/include
  ${extern_dir}
  ${src_dir}
  )

# When I make changes to PlatformWGL the following libraries are
# updated. I'm directly linking with the libraries from the build
# directory because I haven't found a way to only install one
# specific target. When I run `cmake build . --target install`
# all the install commands are executed which takes too long.
if ("${subsys}" STREQUAL "clang")
  list(APPEND roxlu_libs
    ${fila_lib_dir}/filament.lib
    ${fila_build_dir}/libs/math/math.lib
    ${fila_build_dir}/libs/utils/utils.lib
    ${fila_build_dir}/filament/backend/backend.lib
    ${fila_build_dir}/libs/bluegl/bluegl.lib
    
    )
else()
  list(APPEND roxlu_libs
    ${fila_lib_dir}/filament.lib
    ${fila_build_dir}/libs/math/Release/math.lib
    ${fila_build_dir}/libs/utils/Release/utils.lib
    ${fila_build_dir}/filament/backend/Release/backend.lib
    ${fila_build_dir}/libs/bluegl/Release/bluegl.lib
    )
endif()

list(APPEND roxlu_libs
  ${fila_lib_dir}/filabridge.lib
  ${fila_lib_dir}/filaflat.lib
  ${fila_lib_dir}/filamat.lib
  ${fila_lib_dir}/filamat_lite.lib
  ${fila_lib_dir}/filameshio.lib
  ${fila_lib_dir}/geometry.lib
  ${fila_lib_dir}/gltfio.lib
  ${fila_lib_dir}/gltfio_core.lib
  ${fila_lib_dir}/ibl.lib
  ${fila_lib_dir}/image.lib
  ${fila_lib_dir}/matdbg.lib
  ${fila_lib_dir}/meshoptimizer.lib
  ${fila_lib_dir}/rays.lib
  ${fila_lib_dir}/shaders.lib
  ${fila_lib_dir}/smol-v.lib
  ${glfw_install_dir}/lib/glfw3.lib
  Opengl32.lib
  ws2_32.lib
  psapi.lib
  iphlpapi.lib
  gdi32.lib
  )

add_library(myloader STATIC ${extern_dir}/glad/src/glad.c ${src_dir}/OpenGlContext.cpp)
list(APPEND roxlu_libs myloader)

set(test_name test-shared-gl-context)
add_executable(${test_name} ${src_dir}/test-shared-gl-context.cpp)
add_dependencies(${test_name} ${roxlu_deps})
set_property(TARGET ${test_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
target_link_libraries(${test_name} ${roxlu_libs})
install(TARGETS ${test_name} DESTINATION bin)

set(test_name test-backend-config)
add_executable(${test_name} ${src_dir}/test-backend-config.cpp)
add_dependencies(${test_name} ${roxlu_deps})
set_property(TARGET ${test_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
target_link_libraries(${test_name} ${roxlu_libs})
install(TARGETS ${test_name} DESTINATION bin)

set(test_name test-wgl-shared-context)
add_executable(${test_name} ${src_dir}/test-wgl-shared-context.cpp)
add_dependencies(${test_name} ${roxlu_deps})
set_property(TARGET ${test_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
target_link_libraries(${test_name} ${roxlu_libs})
install(TARGETS ${test_name} DESTINATION bin)


