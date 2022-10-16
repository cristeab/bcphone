#!/bin/bash

set -e

CUR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
INSTALL_DIR=$CUR_DIR/../pjproject-macos-install
BCG729_DIR=$CUR_DIR/../bcg729-install
APP_DIR=$CUR_DIR/../bcphone

make clean && make distclean
rm -rf $INSTALL_DIR/*

cp $APP_DIR/tools/config_site_macos.h pjlib/include/pj/config_site.h

./configure --prefix $INSTALL_DIR --disable-ffmpeg --disable-libwebrtc --with-ssl=/usr/local/opt/openssl@1.1 --with-bcg729=$BCG729_DIR

make dep && make
make install
