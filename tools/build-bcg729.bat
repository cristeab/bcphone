rmdir /S /Q  cmake-build

mkdir cmake-build
pushd cmake-build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

"C:\Program Files\CMake\bin\cmake.exe" .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%1
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

cmake --build .
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

cmake --build . --target install
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

popd