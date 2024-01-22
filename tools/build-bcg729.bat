set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
set CMAKE_PATH="C:\Program Files\CMake\bin"

rmdir /S /Q  cmake-build

mkdir cmake-build
pushd cmake-build

call %VS_PATH%"\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

%CMAKE_PATH%"\cmake.exe" .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%1 -DBUILD_SHARED_LIBS=OFF
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