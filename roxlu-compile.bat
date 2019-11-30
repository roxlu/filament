set pwd=%CD%
set bd=%pwd%/roxlu-build
set id=%pwd%/roxlu-installed

if not exist "%bd%" (
   mkdir "%bd%"
)

:: Building with VS2019
cd "%bd%"
cmake -G "Visual Studio 16 2019" ^
      -A X64 ^
      -DCMAKE_INSTALL_PREFIX="%id%" ^
      -DCMAKE_BUILD_TYPE="Release" ^
      ../                                      

if errorlevel 1 (
   echo "Failed to configure."
   exit
)

cmake --build . --target install --config "Release"

:: Trying to create /MD build .. no success
::      -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ^
::      -DCMAKE_CXX_FLAGS="/MD" ^
::      -DCMAKE_CXX_FLAGS_RELEASE="/MD" ^
::      -DCMAKE_C_FLAGS="/MD" ^
::      -DCMAKE_VERBOSE_MAKEFILE="On" ^
