#!/bin/bash

set -e

CUR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
INSTALL_DIR=$CUR_DIR/../bcg729-install
CMAKE_DIR=/Applications/CMake.app/Contents/bin
BUILD_DIR=cmake-build

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
pushd $BUILD_DIR

$CMAKE_DIR/cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DENABLE_TESTS=OFF

$CMAKE_DIR/cmake --build .

$CMAKE_DIR/cmake --build . --target install

popd
