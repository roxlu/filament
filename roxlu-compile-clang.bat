@echo off
set pwd=%CD%
set base_dir=%pwd%/roxlu-clang
set build_dir=%base_dir%/build
set install_dir=%base_dir%/installed

if not exist "%base_dir%" (
   mkdir "%base_dir%"
)

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
      -DUSE_STATIC_CRT=Off ^
      -DENABLE_JAVA=Off ^
      ../..                                      

if errorlevel 1 (
   echo "Failed to configure."
   exit
)

cmake --build . --target install --config "Release" --parallel 12

cd "%pwd%"
