$PJSIP_ARCHIVE = '2.12.1.zip'
$OPENH264_ARCHIVE = 'openh264-2.3.1-win64.dll.bz2'

Write-Output 'Download $PJSIP_ARCHIVE...'
Invoke-WebRequest -Uri "https://github.com/pjsip/pjproject/archive/refs/tags/$PJSIP_ARCHIVE" -OutFile $PJSIP_ARCHIVE
Expand-Archive -Force $PJSIP_ARCHIVE .

Write-Output 'Clone BCG729...'
git clone https://github.com/BelledonneCommunications/bcg729.git

Write-Output 'Download external dependences with conan'
conan install . -if conan-build-info

Write-Output 'Download $OPENH264_ARCHIVE...'
Invoke-WebRequest -Uri "http://ciscobinary.openh264.org/$OPENH264_ARCHIVE" -OutFile $OPENH264_ARCHIVE
7z x -y $OPENH264_ARCHIVE
