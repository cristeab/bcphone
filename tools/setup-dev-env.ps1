$PJSIP_VERSION = '2.12.1'
$OPENH264_VERSION = '2.3.1'
$OPENH264_ARCHIVE = "openh264-$OPENH264_VERSION-win64.dll.bz2"
$INSTALL_DIR = 'deps'

Write-Output "Download $PJSIP_VERSION.zip..."
Invoke-WebRequest -Uri "https://github.com/pjsip/pjproject/archive/refs/tags/$PJSIP_VERSION.zip" -OutFile "$PJSIP_VERSION.zip"
Expand-Archive -Force "$PJSIP_VERSION.zip" .
Copy-Item "config_site_win.h" "pjproject-$PJSIP_VERSION/pjlib/include/pj/config_site.h"

Write-Output "Clone BCG729..."
git clone https://github.com/BelledonneCommunications/bcg729.git

Write-Output "Compile BCG729..."
Copy-Item "build-bcg729.bat" -Destination "bcg729"
Set-Location bcg729
.\build-bcg729.bat '$PSScriptRoot/$INSTALL_DIR'
Set-Location ..

Write-Output "Download vpx, opus and SDL2 libraries with conan"
conan install . -if $INSTALL_DIR

Write-Output "Download $OPENH264_ARCHIVE..."
Invoke-WebRequest -Uri "http://ciscobinary.openh264.org/$OPENH264_ARCHIVE" -OutFile $OPENH264_ARCHIVE
7z x -y $OPENH264_ARCHIVE
New-Item -Path "$INSTALL_DIR" -Name "bin" -ItemType "directory"
Copy-Item "openh264-$OPENH264_VERSION-win64.dll" -Destination "$INSTALL_DIR/bin/"

Invoke-WebRequest -Uri "https://github.com/cisco/openh264/archive/refs/tags/v$OPENH264_VERSION.zip" -OutFile "v$OPENH264_VERSION.zip"
Expand-Archive -Force "v$OPENH264_VERSION.zip" .
