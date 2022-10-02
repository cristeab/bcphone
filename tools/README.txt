# macOS

- build-pjsip-macos.sh: build PJSIP library from sources

- gen-icns.sh: generate icns file

# Windows

- setup-dev-env.ps1: download PSJIP sources and the associated external dependences.
Before using this file install chocolatey from https://chocolatey.org, then run

    choco install conan 7zip

You might also need to set the execution policy for PowerShell scripts running as admin

    Set-ExecutionPolicy -ExecutionPolicy Unrestricted
