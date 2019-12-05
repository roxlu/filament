@echo off
set pwd=%CD%
set base_dir=%pwd%/roxlu-clang
set build_dir=%base_dir%/build
set install_dir=%base_dir%/installed
set runtime=md
set static_flag=Off

:: Iterate over command line arguments
:argsloop

  if "%1" == "" (
    goto :argsloopdone
  )

  if "%1" == "md" (
    set runtime=md
    set static_flag=Off
  )

  if "%1" == "mt" (
    set runtime=mt
    set static_flag=On
  )
  
echo %1
shift
goto :argsloop
:argsloopdone

if not exist "%base_dir%" (
   mkdir "%base_dir%"
)

set build_dir=%build_dir%-%runtime%
set install_dir=%install_dir%-%runtime%

if not exist "%build_dir%" (
   mkdir "%build_dir%"
)

:: Building with Clang + Ninja
cd "%build_dir%"
cmake -G "Ninja" ^
      -DCMAKE_CXX_COMPILER:PATH="C:\Program Files\LLVM\bin\clang-cl.exe" ^
      -DCMAKE_C_COMPILER:PATH="C:\Program Files\LLVM\bin\clang-cl.exe" ^
      -DCMAKE_LINKER:PATH="C:\Program Files\LLVM\bin\lld-link.exe" ^
      -DCMAKE_INSTALL_PREFIX="%install_dir%" ^
      -DCMAKE_BUILD_TYPE="Release" ^
      -DENABLE_JAVA=Off ^
      -DUSE_STATIC_CRT=%static_flag% ^
      ../..                                      

if errorlevel 1 (
   echo "Failed to configure."
   exit
)

cmake --build . --target install --config "Release" --parallel 12 -- -v

cd "%pwd%"
