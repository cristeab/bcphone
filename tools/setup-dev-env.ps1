$PJSIP_ARCHIVE = '2.12.1.zip'

Write-Output 'Download $PJSIP_ARCHIVE...'
Invoke-WebRequest -Uri "https://github.com/pjsip/pjproject/archive/refs/tags/$PJSIP_ARCHIVE" -OutFile $PJSIP_ARCHIVE
Expand-Archive -Force $PJSIP_ARCHIVE .

Write-Output 'Clone BCG729...'
git clone https://github.com/BelledonneCommunications/bcg729.git
