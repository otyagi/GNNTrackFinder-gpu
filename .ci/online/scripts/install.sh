#!/bin/bash

set -e

srcdir=cbmroot
build_type=RelWithDebInfo
install_prefix=/opt/cbm/cbmroot
fairsoft_path=/opt/cbm/fairsoft
enable_hip=ON
rocm_root=/opt/rocm
hip_archs="gfx906;gfx908" # MI50;MI100
n_jobs=16

cmake_args="-DCMAKE_INSTALL_PREFIX=$install_prefix \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DSIMPATH=$fairsoft_path \
    -DXPU_ENABLE_HIP=$enable_hip \
    -DXPU_ROCM_ROOT=$rocm_root \
    -DXPU_HIP_ARCH=$hip_archs \
"

ln -s ../cmake algo/cmake
cmake -S algo -B build $cmake_args
cmake --build build -j$n_jobs --target install
