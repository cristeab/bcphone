# Introduction

Cloud based softphone using Qt/QML application framework and PJSIP library.

Supported plaforms:

- macOS

- Windows (planned)

- Ubuntu (planned)

- iOS (planned)

- Android (planned)

Features:

- audio and video calls

- call history

- contacts

- chat

- supported SIP transports: UDP, TCP and TLS

- supported media transports: RTP and SRTP

- supported audio codecs: G729, Opus, PCMU, PCMA, G722, Speex, etc.

- supported video codecs: H264 and VP8

- support for outbound proxy

- presence subscription

- conference mode

- call recording

- blind and supervised transfers


# Compilation Instructions

Requirements:

- [Qt v6.2+](https://www.qt.io)

- [PJSIP library v2.12+](https://github.com/pjsip/pjproject/releases)

- [cmake v3.17+](https://cmake.org/download/)


Compilation Steps:

- build PJSIP library following the instructions found here: https://github.com/pjsip/pjproject

- open CMakeLists.txt file and make sure that the libraries paths (e.g. PJSIP_ROOT_DIR) point to the correct locations

- build the application using the provided script for the target platform (e.g. build-macos.sh)


# Screenshots

![Dialpad](screenshots/dialpad.png?raw=true "Dialpad")

![Settings](screenshots/settings1.png?raw=true "Settings")

![Codecs](screenshots/settings2.png?raw=true "Codecs")
