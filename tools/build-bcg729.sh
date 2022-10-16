#!/bin/bash

set -e

INSTALL_DIR=../bcg729-install
CMAKE_DIR=/Applications/CMake.app/Contents/bin

mkdir cmake-build
pushd cmake-build

$CMAKE_DIR/cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DENABLE_TESTS=OFF

$CMAKE_DIR/cmake --build .

$CMAKE_DIR/cmake --build . --target install

popd
