#!/bin/sh

# -------------------------------------------------------------
# 
# This script compiles filament and installs it into
# `${PWD}/installed`. The samples are not installed though. I
# created this test while experimenting with fading in/out a
# emissive/bloom material.
#
# LOG:
#
#    2020-06-17:
#    - Created this file to debug fade in/out bloom.
#
# -------------------------------------------------------------

build_type="Release"
build_dir=${PWD}/build.${build_type}
asset_dir=${PWD}/assets
mat_dir=${PWD}/samples/materials
install_dir=${PWD}/installed

# -------------------------------------------------------------

if [ ! -d ${build_dir} ] ; then
    mkdir ${build_dir}
fi

# -------------------------------------------------------------

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export CXXFLAGS=-stdlib=libc++

cd ${build_dir}
cmake -DCMAKE_INSTALL_PREFIX=${install_dir} \
      -DCMAKE_CXX_COMPILER=${CXX} \
      -DCMAKE_C_COMPILER=${CC} \
      -DCMAKE_CXX_FLAGS=${CXXFLAGS} \
      -DCMAKE_BUILD_TYPE=${build_type} \
      -DFILAMENT_ENABLE_JAVA=OFF \
      ..

if [ $? -ne 0 ] ; then
    echo "Failed to configure."
    exit
fi

# -------------------------------------------------------------

target=matmesh

if [ ! -f ${install_dir}/bin/matc ] ; then
    target=install
fi

cmake --build . --target ${target} --parallel 16

if [ $? -ne 0 ] ; then
    echo "Failed to build."
    exit
fi

# -------------------------------------------------------------

# Create the filamesh + filamat files. (done every run)
cd ${install_dir}/bin/
./filamesh -c ${asset_dir}/models/cube/cube-v2.obj ${build_dir}/cube-v2.filamesh
./matc -a opengl -p desktop -o ${build_dir}/glow.filamat ${mat_dir}/glow.mat

# -------------------------------------------------------------

# Run `matmesh` 
cd ${build_dir}/samples
./matmesh --material ${build_dir}/glow.filamat --mesh ${build_dir}/cube-v2.filamesh
#./matmesh --material ${build_dir}/glow.filamat --mesh ./generated/resources/suzanne.filamesh

