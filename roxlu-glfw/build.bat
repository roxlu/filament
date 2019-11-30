@echo off
set pwd=%CD%
set bd=%pwd%/roxlu-build
set id=%pwd%/roxlu-installed
set ed=%pwd%/extern

if not exist "%bd%" (
   mkdir "%bd%"
)

if not exist "%ed%" (
  mkdir "%ed%"
)     

:: Compile GLFW
if not exist "%ed%/glfw" (

   cd "%ed%"

   git clone https://github.com/glfw/glfw.git
   cd glfw
   
   mkdir build
   cd build

   cmake -G "Visual Studio 16 2019" ^
         -A X64 ^
         -DCMAKE_INSTALL_PREFIX="%id%" ^
         -DCMAKE_BUILD_TYPE="Release" ^
         -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" ^
         -DUSE_MSVC_RUNTIME_LIBRARY_DLL=Off ^
         ..

  if errorlevel 1 (
      echo "Failed to configure GLFW"
      exit
  )

  cmake --build . --target install --config "Release"
  if errorlevel 1 (
     echo "Failed to build GLFW"
     exit
  )
)

if not exist "%pwd%/../roxlu-installed" (
   echo "The Filament install dir does not exist. Did you compile filament?"
   exit
)

:: Rebuild the backend target as that's what we need when making
:: changes to the GL backend.
cd "%pwd%/../roxlu-build"
cmake --build . --target "backend" --config "Release"
if errorlevel 1 (
  echo "Failed to build Filament."
  exit 
)

:: Building with VS2019
cd "%bd%"
cmake -G "Visual Studio 16 2019" ^
      -A X64 ^
      -DCMAKE_INSTALL_PREFIX="%id%" ^
      -DCMAKE_BUILD_TYPE="Release" ^
      -DCMAKE_VERBOSE_MAKEFILE=On ^
      ../                                      

if errorlevel 1 (
   echo "Failed to configure."
   exit
)

cmake --build . --target install --config "Release"
if errorlevel 1 (
   echo "Failed to build."
   exit
)

cd "%id%"
cd bin
test-shared-gl-context

