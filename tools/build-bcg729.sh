#!/bin/bash

set -e

INSTALL_DIR=../bcg729-install

mkdir cmake-build
pushd cmake-build

cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DENABLE_TESTS=OFF

cmake --build .

cmake --build . --target install

popd
