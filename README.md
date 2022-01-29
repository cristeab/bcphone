# Introduction

Cloud based softphone using Qt/QML and PJSIP library.

Supported plaforms:

- macOS

- Windows (planned)

- Ubuntu (planned)

- iOS (planned)

- Android (planned)


# Requirements

- [Qt v6.2+](https://www.qt.io)

- [PJSIP library v2.11+](https://github.com/pjsip/pjproject/releases)

- [cmake v3.17+](https://cmake.org/download/)


# Compilation Instructions

- build PJSIP library following the instructions found here: https://github.com/pjsip/pjproject

- open CMakeLists.txt file and make sure that the libraries paths (e.g. PJSIP_ROOT_DIR) point to the correct locations

- build the application using the provided script for the target platform (e.g. build-macos.sh)
