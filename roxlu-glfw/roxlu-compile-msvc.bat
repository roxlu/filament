@echo off

set pwd=%CD%
set base_dir=%pwd%/roxlu-msvc
set build_dir=%base_dir%/build
set install_dir=%base_dir%/installed
set extern_dir=%base_dir%/extern
set fila_base_dir=%pwd%/../roxlu-msvc/

if not exist "%base_dir%" (
   mkdir "%base_dir%"
)

if not exist "%build_dir%" (
  mkdir "%build_dir%"
)     

if not exist "%fila_base_dir%" (
   echo "The Filament install dir does not exist. Did you compile filament?"
   goto:end
)

if not exist "%pwd%/../roxlu-installed" (
   echo "The Filament install dir does not exist. Did you compile filament?"
   exit
)

:: Rebuild the backend target as that's what we need when making
:: changes to the GL backend.
cd "%fila_base_dir%/build/"
cmake --build . --target "backend" --config "Release"
if errorlevel 1 (
   echo "Failed to build Filament."
   goto:end
)

:: Building with VS2019
cd "%bd%"
cmake -G "Visual Studio 16 2019" ^
      -A X64 ^
      -DCMAKE_INSTALL_PREFIX="%install_dir%" ^
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

