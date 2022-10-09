$PJSIP_VERSION = '2.12.1'
$INSTALL_DIR = 'precompiled'

Write-Output "Download $PJSIP_VERSION.zip..."
Invoke-WebRequest -Uri "https://github.com/pjsip/pjproject/archive/refs/tags/$PJSIP_VERSION.zip" -OutFile "$PJSIP_VERSION.zip"
Expand-Archive -Force "$PJSIP_VERSION.zip" .
Copy-Item "config_site_win.h" "pjproject-$PJSIP_VERSION/pjlib/include/pj/config_site.h"

Write-Output "Clone BCG729..."
git clone https://github.com/BelledonneCommunications/bcg729.git

Write-Output "Compile BCG729..."
Copy-Item "build-bcg729.bat" -Destination "bcg729"
Set-Location bcg729
.\build-bcg729.bat "$PSScriptRoot/$INSTALL_DIR"
Set-Location ..

Write-Output "Download vpx, opus, openh264 and SDL2 libraries with conan"
conan install . -if $INSTALL_DIR
