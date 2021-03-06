cmake_minimum_required(VERSION 3.10)
project(bluegl ASM)

set(TARGET              bluegl)
set(PUBLIC_HDR_DIR      include)

if (WIN32)
    enable_language(ASM_MASM)
    set_property(SOURCE src/BlueGLCoreWindowsImpl.S PROPERTY LANGUAGE ASM_MASM)
endif()

# public headers are always in include/${TARGET}/
file(GLOB_RECURSE PUBLIC_HDRS ${PUBLIC_HDR_DIR}/${TARGET}/*.h)

# list each source file individually
set(SRCS
    src/BlueGL.cpp
)

if (WIN32)
    set(SRCS ${SRCS} src/BlueGLWindows.cpp)
    set(SRCS ${SRCS} src/BlueGLCoreWindowsImpl.S)
elseif (APPLE AND NOT IOS)
    set(SRCS ${SRCS} src/BlueGLDarwin.cpp)
    set(SRCS ${SRCS} src/BlueGLCoreDarwinImpl.S)
elseif(LINUX)
    set(SRCS ${SRCS} src/BlueGLLinux.cpp)
    set(SRCS ${SRCS} src/BlueGLCoreLinuxImpl.S)
else()
    message(FATAL_ERROR "Platform not supported. BlueGL supports Windows, Linux, and MacOS X.")
endif()

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

# Only linux, clang doesn't want to use a shared library that is not PIC.
# /usr/bin/ld: ../bluegl/libbluegl.a(BlueGL.cpp.o): relocation R_X86_64_32S
# against `.bss' can not be used when making a shared object; recompile with -fPIC
if (LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# specify where our headers are
include_directories(${PUBLIC_HDR_DIR})

# we're building a library
add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

# specify where the public headers of this library are
target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})

if (WIN32)
    target_link_libraries(${TARGET} PRIVATE opengl32 gdi32)
endif()

install(TARGETS ${TARGET} ARCHIVE DESTINATION lib/${DIST_DIR})

# Build the tests...
add_executable(test_${TARGET}
    tests/OpenGLSupport.cpp
    tests/OpenGLSupport.hpp
    tests/test_bluegl.cpp)

if (LINUX)
    target_link_libraries(test_${TARGET} PUBLIC dl)
endif()

# and we're linking against the libraries below, importing their public headers
target_link_libraries(test_${TARGET} LINK_PUBLIC ${TARGET})
target_link_libraries(test_${TARGET} LINK_PUBLIC gtest)
