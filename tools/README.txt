# macOS

- build-pjsip-macos.sh: build PJSIP library from sources

- gen-icns.sh: generate icns file

# Windows

In order to compile PJSIP library and the external dependecies use 'setup-dev-env.ps1'.
Create a folder '3rdparty' on the same folder where the application sources are and copy in it
'setup-dev-env.ps1', 'build-bcg729.bat', 'conanfile.txt' and 'config_site_win.h'.

- setup-dev-env.ps1: download PSJIP sources and the associated external dependences.
Before using this file install chocolatey from https://chocolatey.org, then run

    choco install conan 7zip

You might also need to set the execution policy for PowerShell scripts running as admin

    Set-ExecutionPolicy -ExecutionPolicy Unrestricted

Once all dependences downloaded, open in VS the sln file found in pjproject folder, select release static build for x64 and compile. After the first
compilation several errors about unfound header files will be generated. Solve them by specifying the header paths from the installed dependences.
