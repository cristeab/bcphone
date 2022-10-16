#!/bin/bash

compile_app=false
sign_installer=false
old_mac=false
if [ "$#" -eq 0 ] ; then
    compile_app=true
    sign_installer=true
else
    while getopts ":cso" opt; do
      case ${opt} in
        c ) compile_app=true
            echo "Compile the application"
          ;;
        s ) sign_installer=true
            echo "Sign the application and generate the installer"
          ;;
        o ) old_mac=true
              echo "Using old macOS platform"
            ;;
        \? ) echo "Usage: $0 [-c] [-s] [-o]"
             echo "-c compile the application"
             echo "-s sign the application and generate the installer"
             echo "-o old macOS platform"
             exit 1
          ;;
      esac
    done
fi

QT_VER=6.4.0
APP_NAME=BCPhone
APP_IDENTIFIER="com.cristeab.bcphone"
MAJOR_VERSION=1
MINOR_VERSION=0
CMAKE_PATH=/Applications/CMake.app/Contents/bin

if [ "$compile_app" = true ] ; then

    echo "QT version $QT_VER"

    rm -rf build

    mkdir build
    cd build

    if [ "$old_mac" = true ] ; then
        $CMAKE_PATH/cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/$QT_VER/macos -DPRODUCTION_BUILD=off -DOLD_MAC=on
    else
        $CMAKE_PATH/cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/$QT_VER/macos -DPRODUCTION_BUILD=off
    fi
    $CMAKE_PATH/cmake --build . -j
    $CMAKE_PATH/cmake --build . --target pack
    cd ..

    # copy inside the bundle additional libs
    if [ "$old_mac" = true ] ; then
        cp /usr/local/lib/gcc/10/libgcc_s.1.dylib build/$APP_NAME.app/Contents/Frameworks
    fi

    # fix libs paths
    #install_name_tool -change /usr/local/Cellar/nettle/3.6/lib/libnettle.8.dylib "@executable_path/../Frameworks/libnettle.8.dylib" build/Airbytes.app/Contents/Frameworks/libhogweed.6.dylib

fi # compile app

if [ "$sign_installer" = true ] ; then

    # sign bundle AFTER macdeploy
    codesign --strict --timestamp --force --verify --verbose --deep \
             --entitlements platform/macos/Entitlements.plist \
             --sign "Developer ID Application: Bogdan Cristea" \
             --options runtime ./build/$APP_NAME.app

    # check the signing
    codesign --verify --verbose=4 --deep --strict ./build/$APP_NAME.app
    if [ $? -ne 0 ]; then
        exit $?
    fi

    # create installer
    rm -f build/$APP_NAME.dmg
    $HOME/node_modules/appdmg/bin/appdmg.js CustomDmg.json build/$APP_NAME.dmg

    if [ -z "$APP_PWD" ]; then
        echo Notarization password is not set
        exit 0
    fi

    echo "Upload installer to Apple servers"
    xcrun altool --notarize-app \
                 --primary-bundle-id $APP_IDENTIFIER \
                 --username cristeab@gmail.com \
                 --password $APP_PWD \
                 --asc-provider "FAARUB626Q" \
                 --file ./build/$APP_NAME.dmg | tee build/notarization.log
    UUID=`cat build/notarization.log | grep -Eo '\w{8}-(\w{4}-){3}\w{12}$'`
    if [ $? -ne 0 ]; then
        exit $?
    fi

    echo -n "Check notarization result "
    while true; do
        xcrun altool --notarization-info $UUID \
                     --username cristeab@gmail.com \
                     --password $APP_PWD &> build/notarization.log
        r=`cat build/notarization.log`
        t=`echo "$r" | grep "success"`
        f=`echo "$r" | grep "invalid"`
        if [[ "$t" != "" ]]; then
            break
        fi
        if [[ "$f" != "" ]]; then
            echo "failure"
            echo "$r"
            exit 1
        fi
        echo -n "."
        sleep 30
    done
    echo "success"

    # once the notarization is successful
    xcrun stapler staple -v build/$APP_NAME.dmg

fi # sign installer

# change app name by including the version
mv build/$APP_NAME.dmg build/$APP_NAME-$MAJOR_VERSION.$MINOR_VERSION.dmg
