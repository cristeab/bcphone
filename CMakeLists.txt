project("BCPhone" CXX)
cmake_minimum_required(VERSION 3.20)

include(cmake/internal_utils.cmake)
get_app_version(SOFTPHONE_VERSION "1.0")

option(PRODUCTION_BUILD "Build the application for production" OFF)
if (PRODUCTION_BUILD)
    add_definitions(-DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT -DQT_NO_INFO_OUTPUT)
endif()

if (APPLE)
    if (NOT PJSIP_ROOT_DIR)
        set(PJSIP_ROOT_DIR "${CMAKE_SOURCE_DIR}/../pjproject-macos-install")
    endif()
    set (ENV{PKG_CONFIG_PATH} ${PJSIP_ROOT_DIR}/lib/pkgconfig)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@1.1")
elseif (WIN32)
    set (DEPS_DIR "${CMAKE_SOURCE_DIR}/../3rdparty")
    set (PJSIP_ROOT_DIR "${DEPS_DIR}/pjproject-2.12.1")
    set (PJSIP_INCLUDE_DIRS ${PJSIP_ROOT_DIR}/pjsip/include
                            ${PJSIP_ROOT_DIR}/pjlib/include
                            ${PJSIP_ROOT_DIR}/pjlib-util/include
                            ${PJSIP_ROOT_DIR}/pjmedia/include
                            ${PJSIP_ROOT_DIR}/pjnath/include)
    set (PJSIP_LIBRARIES "libpjproject-x86_64-x64-vc14-Release-Static.lib")

    set (PRECOMPILED_ROOT_DIR "${DEPS_DIR}/precompiled")
    set (PRECOMPILED_LIBRARIES "openh264.lib;opus.lib;bcg729.lib;SDL2-static.lib;Setupapi.lib;Version.lib;Imm32.lib;vpxmd.lib")

    set (OPENSSL_ROOT_DIR "${CMAKE_PREFIX_PATH}/../../Tools/OpenSSL/Win_x64")
    set (OPENSSL_LIBRARIES "libcrypto.lib;libssl.lib")
else ()
    message(FATAL "Unsupported platform")
endif()

#set from the beginning to include into config.h
set(CPACK_PACKAGE_VENDOR "Bogdan Cristea")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Qt6 6.4 COMPONENTS
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
        MACOSX_BUNDLE_INFO_STRING ${PROJECT_NAME}
        MACOSX_BUNDLE_ICON_FILE "bcphone.icns"
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${SOFTPHONE_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2022 Bogdan Cristea. All rights reserved")
    target_include_directories (${PROJECT_NAME} PRIVATE src ${PJSIP_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_ROOT_DIR}/lib)
    string (REPLACE ";" " " PJSIP_STATIC_LDFLAGS_STR "${PJSIP_STATIC_LDFLAGS}")
    target_link_libraries (${PROJECT_NAME} Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::Svg
                                            ${PJSIP_STATIC_LDFLAGS_STR})

    #custom plist file
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in)
elseif (WIN32)
    set (PLATFORM_SRCS platform/win/win_utils.cpp)
    add_executable(${PROJECT_NAME} WIN32 ${SRCS} ${PLATFORM_SRCS} ${RSCS} "${CMAKE_SOURCE_DIR}/img/app.rc")
    target_include_directories (${PROJECT_NAME} PRIVATE src ${PJSIP_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
    target_link_directories(${PROJECT_NAME} PRIVATE "${PJSIP_ROOT_DIR}/lib;${PRECOMPILED_ROOT_DIR}/lib;${OPENSSL_ROOT_DIR}/lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::Svg ${PJSIP_LIBRARIES} ${PRECOMPILED_LIBRARIES} ${OPENSSL_LIBRARIES})
endif()

#installer setup
if (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cloud based softphone")
    set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_SOURCE_DIR}/README.md)
    set(CPACK_PACKAGE_VERSION ${SOFTPHONE_VERSION})
    set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
    set(CPACK_PACKAGE_CONTACT "cristeab@gmail.com")
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
