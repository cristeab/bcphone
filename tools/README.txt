# macOS

- build-pjsip-macos.sh: build PJSIP library from sources

- gen-icns.sh: generate icns file

# Windows

- setup-dev-env.ps1: download PSJIP sources and the associated external dependences.
Before using this file install chocolatey from https://chocolatey.org, then run

    choco install conan 7zip

You might also need to set the execution policy for PowerShell scripts running as admin

    Set-ExecutionPolicy -ExecutionPolicy Unrestricted

Once all dependences downloaded, open in VS the sln file found in pjproject folder, select release static build for x64 and compile. After the first
compilation several errors about unfound header files will be generated. Solve them by specifying the header paths from the installed dependences.
