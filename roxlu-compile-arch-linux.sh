#!/bin/sh
d=${PWD}

if [ ! -d ${d}/out ] ; then
    mkdir ${d}/out
fi

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export CXXFLAGS="-O2 -g -stdlib=libc++ -fno-builtin "
export LDFLAGS="-stdlib=libc++" 

cd out
cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=${d}/installed \
      ../


if [ ! $? -eq 0 ] ; then
    echo "Failed to configure"
    exit
fi

ninja
