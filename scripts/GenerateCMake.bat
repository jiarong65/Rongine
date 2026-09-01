@echo off
setlocal

call "D:\vs\root\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%

set "PATH=D:\llvm\bin;D:\env\ninja-win;%PATH%"

set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build-clang"

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_C_COMPILER=D:/llvm/bin/clang.exe ^
  -DCMAKE_CXX_COMPILER=D:/llvm/bin/clang++.exe ^
  -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%
