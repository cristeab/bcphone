project("BCphone" CXX)
cmake_minimum_required(VERSION 3.17)

set(SOFTPHONE_VERSION "0.1")

if (APPLE)
    if (NOT PJSIP_ROOT_DIR)
        set(PJSIP_ROOT_DIR "${CMAKE_SOURCE_DIR}/../build-macos")
    endif()
    set (PJSIP_INCLUDE_DIRS "${PJSIP_ROOT_DIR}/include")
    set (PJSIP_LIBRARIES "libg7221codec-x86_64-apple-darwin21.3.0.a;libpjlib-util-x86_64-apple-darwin21.3.0.a;libpjmedia-x86_64-apple-darwin21.3.0.a;libpjsip-x86_64-apple-darwin21.3.0.a;libspeex-x86_64-apple-darwin21.3.0.a;libgsmcodec-x86_64-apple-darwin21.3.0.a;libpjmedia-audiodev-x86_64-apple-darwin21.3.0.a;libpjnath-x86_64-apple-darwin21.3.0.a;libpjsua-x86_64-apple-darwin21.3.0.a;libsrtp-x86_64-apple-darwin21.3.0.a;libilbccodec-x86_64-apple-darwin21.3.0.a;libpjmedia-codec-x86_64-apple-darwin21.3.0.a;libpjsip-simple-x86_64-apple-darwin21.3.0.a;libpjsua2-x86_64-apple-darwin21.3.0.a;libyuv-x86_64-apple-darwin21.3.0.a;libpj-x86_64-apple-darwin21.3.0.a;libpjmedia-videodev-x86_64-apple-darwin21.3.0.a;libpjsip-ua-x86_64-apple-darwin21.3.0.a;libresample-x86_64-apple-darwin21.3.0.a")

    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules/ ${CMAKE_MODULE_PATH})
    find_package(CoreFoundation REQUIRED)
    find_package(AudioToolbox REQUIRED)
    find_package(CoreAudio REQUIRED)
    find_package(CoreServices REQUIRED)
    find_package(CoreMedia REQUIRED)
    find_package(Foundation REQUIRED)
    find_package(SDL2 REQUIRED)
elseif (WIN32)
    if (NOT PJSIP_ROOT_DIR)
        set (PJSIP_ROOT_DIR "${CMAKE_SOURCE_DIR}/../pjproject-2.11.1")
    endif ()
    set (PJSIP_INCLUDE_DIRS "${PJSIP_ROOT_DIR}/pjsip/include;${PJSIP_ROOT_DIR}/pjlib/include;${PJSIP_ROOT_DIR}/pjlib-util/include;${PJSIP_ROOT_DIR}/pjmedia/include;${PJSIP_ROOT_DIR}/pjnath/include")
    set (PJSIP_LIBRARIES "libpjproject-x86_64-x64-vc14-Release-Static.lib")

    set (VPX_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rdparty/libvpx")

    set (OPENH264_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rdparty/openh264")
    set (OPENH264_LIBRARIES "openh264.lib")

    set (OPENSSL_ROOT_DIR "${CMAKE_PREFIX_PATH}/../../Tools/OpenSSL/Win_x64")
    set (OPENSSL_LIBRARIES "libcrypto.lib;libssl.lib")

    set (OPUS_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rdparty/opus")
    set (OPUS_LIBRARIES "opus.lib")

    set (BCG729_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rdparty/bcg729")
    set (BCG729_LIBRARIES "bcg729.lib")

    set (SDL2_ROOT_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2-2.0.18")
    set (SDL2_LIBRARIES "version.lib;Imm32.lib;Setupapi.lib")
else ()
    message(FATAL "Unsupported platform")
endif()

option(PRODUCTION_BUILD "Build the application for production" OFF)
if (PRODUCTION_BUILD)
    add_definitions(-DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT -DQT_NO_INFO_OUTPUT)
endif()

#set from the beginning to include into config.h
set(CPACK_PACKAGE_VENDOR "Bogdan Cristea")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Qt6 6.2 COMPONENTS
                        Core
                        Qml
                        Quick
                        Widgets
                        Svg
                        Gui
                        Multimedia
                        Sql
                       REQUIRED)

#set version in config file
configure_file(${CMAKE_SOURCE_DIR}/src/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

file (GLOB_RECURSE SRCS src/*.cpp)
qt6_add_resources (RSCS res.qrc)

if (APPLE)
    set (PLATFORM_SRCS platform/macos/device_notifier.mm platform/macos/dock_click_handler.mm)
    add_executable ("${PROJECT_NAME}" MACOSX_BUNDLE ${SRCS} ${PLATFORM_SRCS} ${RSCS}
        "${CMAKE_SOURCE_DIR}/img/bcphone.icns")
    set_source_files_properties ("${CMAKE_SOURCE_DIR}/img/bcphone.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cristeab.bcphone"
        MACOSX_BUNDLE_INFO_STRING "BCphone"
        MACOSX_BUNDLE_ICON_FILE "bcphone.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "BCphone"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2022 Bogdan Cristea. All rights reserved")
    target_include_directories (${PROJECT_NAME} PRIVATE src ${PJSIP_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE ${PJSIP_ROOT_DIR}/lib)
    target_link_libraries (${PROJECT_NAME} Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::Svg
        ${PJSIP_LIBRARIES} ${CoreFoundation_LIBRARIES}
        ${AudioToolbox_LIBRARIES} ${CoreAudio_LIBRARIES} ${CoreServices_LIBRARIES}
        ${CoreMedia_LIBRARIES} ${Foundation_LIBRARIES} ${SDL2_LIBRARIES} ${SWSCALE_LIBRARIES}
        /usr/local/lib/libopencore-amrnb.a
        /usr/local/opt/openssl@1.1/lib/libcrypto.a
        /usr/local/opt/openssl@1.1/lib/libssl.a
        /usr/local/Cellar/opus/1.3.1/lib/libopus.0.dylib)

    #custom plist file
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in)
elseif (WIN32)
    set (PLATFORM_SRCS src/platform/win_utils.cpp)
    add_executable(${PROJECT_NAME} WIN32 ${SRCS} ${PLATFORM_SRCS} ${RSCS} "${CMAKE_SOURCE_DIR}/img/app.rc")
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
    target_link_directories(${PROJECT_NAME} PRIVATE "${PJSIP_ROOT_DIR}/lib;${VPX_ROOT_DIR}/lib;${OPENH264_ROOT_DIR}/lib;${OPENSSL_ROOT_DIR}/lib;${OPUS_ROOT_DIR}/lib;${BCG729_ROOT_DIR}/lib;${SDL2_ROOT_DIR}/lib/x64")
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::Svg Qt6::QuickWidgets ${PJSIP_LIBRARIES} ${OPENH264_LIBRARIES} ${OPENSSL_LIBRARIES} ${OPUS_LIBRARIES} ${BCG729_LIBRARIES} ${SDL2_LIBRARIES})
endif()

#installer setup
if (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Autoanswer Softphone")
    set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_SOURCE_DIR}/README.md)
    set(CPACK_PACKAGE_VERSION ${SOFTPHONE_VERSION})
    set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)
    set(CPACK_PACKAGE_CONTACT "TODO")
    set(CPACK_STRIP_FILES ${PROJECT_NAME})

    if (APPLE)
        add_custom_target(pack COMMAND ${CMAKE_PREFIX_PATH}/bin/macdeployqt
            ${PROJECT_NAME}.app
            -qmldir=${CMAKE_SOURCE_DIR}/qml
            -always-overwrite
            -dmg
            -appstore-compliant)
    elseif (WIN32)
        set(VC_REDIST_FILENAME "vc_redist.x64.exe")
        set(CPACK_GENERATOR "NSIS64")
        set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
        set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${PROJECT_NAME}.exe")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
        set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/img/logo.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/img/logo.ico")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\img\\\\logo.bmp")
        set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\${VC_REDIST_FILENAME}\\\" /install /passive /norestart'
             CreateShortCut \\\"$DESKTOP\\\\${PROJECT_NAME}.lnk\\\" \\\"$INSTDIR\\\\${PROJECT_NAME}.exe\\\" \\\"\\\" \\\"$INSTDIR\\\\logo.ico\\\"
             CreateShortCut \\\"$SMPROGRAMS\\\\${PROJECT_NAME}.lnk\\\" \\\"$INSTDIR\\\\${PROJECT_NAME}.exe\\\" \\\"\\\" \\\"$INSTDIR\\\\logo.ico\\\"")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete \\\"$DESKTOP\\\\${PROJECT_NAME}.lnk\\\"
             Delete \\\"$SMPROGRAMS\\\\${PROJECT_NAME}.lnk\\\"
             ExecWait '\\\"$INSTDIR\\\\${PROJECT_NAME}\\\" clear'")

        find_program(WINDEPLOYQT windeployqt PATHS ${QT5_ROOT_PATH}/bin/)
        install(PROGRAMS "${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}.exe" DESTINATION .)
        install(FILES ${CMAKE_SOURCE_DIR}/img/logo.ico DESTINATION .)
        install(FILES ${OPENSSL_ROOT_DIR}/bin/libcrypto-1_1-x64.dll DESTINATION .)
        install(FILES ${OPENSSL_ROOT_DIR}/bin/libssl-1_1-x64.dll DESTINATION .)
        install(FILES ${OPENH264_ROOT_DIR}/lib/openh264.dll DESTINATION .)
        install(FILES ${SDL2_ROOT_DIR}/lib/x64/SDL2.dll DESTINATION .)

        add_custom_target(windeployqt ALL
            DEPENDS ${PROJECT_NAME}.exe
            COMMAND ${WINDEPLOYQT}
            --dir ${PROJECT_BINARY_DIR}/deploy
            --release
            --compiler-runtime
            --qmldir ${PROJECT_SOURCE_DIR}/qml
            ${PROJECT_NAME}.exe
            COMMENT "Preparing Qt runtime dependencies")
        install(DIRECTORY ${PROJECT_BINARY_DIR}/deploy/ DESTINATION .)
    endif ()

    include(CPack)
endif()
