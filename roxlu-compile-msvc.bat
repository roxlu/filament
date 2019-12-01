@echo off
set pwd=%CD%
set base_dir=%pwd%/roxlu-msvc
set build_dir=%base_dir%/build
set install_dir=%base_dir%/installed

if not exist "%base_dir%" (
   mkdir "%base_dir%"
)

if not exist "%build_dir%" (
   mkdir "%build_dir%"
)

:: Building with VS2019
cd "%build_dir%"
cmake -G "Visual Studio 16 2019" ^
      -A X64 ^
      -DCMAKE_INSTALL_PREFIX="%install_dir%" ^
      -DCMAKE_BUILD_TYPE="Release" ^
      ../..
      
if errorlevel 1 (
   echo "Failed to configure."
   goto:eof
)

cmake --build . --target install --config "Release" --parallel 12

:: Trying to create /MD build .. no success
::      -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ^
::      -DCMAKE_CXX_FLAGS="/MD" ^
::      -DCMAKE_CXX_FLAGS_RELEASE="/MD" ^
::      -DCMAKE_C_FLAGS="/MD" ^
::      -DCMAKE_VERBOSE_MAKEFILE="On" ^
