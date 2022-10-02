$PJSIP_ARCHIVE = '2.12.1.zip'
$OPENH264_VERSION = '2.3.1'
$OPENH264_ARCHIVE = "openh264-$OPENH264_VERSION-win64.dll.bz2"
$INSTALL_DIR = deps

Write-Output "Download $PJSIP_ARCHIVE..."
Invoke-WebRequest -Uri "https://github.com/pjsip/pjproject/archive/refs/tags/$PJSIP_ARCHIVE" -OutFile $PJSIP_ARCHIVE
Expand-Archive -Force $PJSIP_ARCHIVE .

Write-Output "Clone BCG729..."
git clone https://github.com/BelledonneCommunications/bcg729.git

Write-Output "Compile BCG729..."
Copy-Item "build-bcg729.bat" -Destination "bcg729"
Set-Location bcg729
.\build-bcg729.bat '$PSScriptRoot/$INSTALL_DIR'

Write-Output "Download vpx, opus and SDL2 libraries with conan"
conan install . -if $INSTALL_DIR

Write-Output "Download $OPENH264_ARCHIVE..."
Invoke-WebRequest -Uri "http://ciscobinary.openh264.org/$OPENH264_ARCHIVE" -OutFile $OPENH264_ARCHIVE
7z x -y $OPENH264_ARCHIVE
Copy-Item "openh264-$OPENH264_VERSION-win64.dll" -Destination "$INSTALL_DIR/bin"
