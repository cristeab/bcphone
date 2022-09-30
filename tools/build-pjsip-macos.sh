#!/bin/bash

CUR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TARGET_DIR=$CUR_DIR/../build-macos

make clean && make distclean
rm -rf $TARGET_DIR/*

./configure --prefix $TARGET_DIR --disable-ffmpeg --disable-libwebrtc --with-ssl=/usr/local/opt/openssl@3 --with-bcg=$CUR_DIR/../bcg729-install

cp pjlib/include/pj/config_site_sample.h pjlib/include/pj/config_site.h

sed -i '' '22i\
#define PJ_CONFIG_MINIMAL_SIZE\
#define PJMEDIA_HAS_VIDEO 1\
#define PJMEDIA_HAS_FFMPEG 1\
#define PJMEDIA_HAS_OPUS_CODEC 1
' pjlib/include/pj/config_site.h

make dep && make
make install
