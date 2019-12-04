@echo off
set pwd=%CD%
set base_dir=%pwd%/roxlu-clang
set build_dir=%base_dir%/build
set install_dir=%base_dir%/installed
set extern_dir=%base_dir%/extern
set fila_base_dir=%pwd%/../roxlu-clang/

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

:: Rebuild the backend target as that's what we need when making
:: changes to the GL backend.
cd "%fila_base_dir%/build/"
cmake --build . --target "backend" --config "Release"
if errorlevel 1 (
   echo "Failed to build Filament."
   goto:end
)

cd "%build_dir%"

cmake -G "Ninja" ^
      -DCMAKE_INSTALL_PREFIX="%install_dir%" ^
      -DCMAKE_CXX_COMPILER:PATH="C:\Program Files\LLVM\bin\clang-cl.exe" ^
      -DCMAKE_C_COMPILER:PATH="C:\Program Files\LLVM\bin\clang-cl.exe" ^
      -DCMAKE_LINKER:PATH="C:\Program Files\LLVM\bin\lld-link.exe" ^
      -DCMAKE_BUILD_TYPE="Release" ^
      -DCMAKE_VERBOSE_MAKEFILE=On ^
      ../..
      
if errorlevel 1 (
   echo "Failed to configure."
   goto:end
)

cmake --build . --target install --config "Release" --parallel 12 -- -v

if errorlevel 1 (
   echo "Failed to build."
   goto:end     
)

cd "%install_dir%"
cd bin
:: test-shared-gl-context
:: test-backend-config
:: test-wgl-shared-context
:: test-backend-config-roxlu
test-backend-config-fbo-roxlu

:end
cd "%pwd%"
