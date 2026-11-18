@echo off

set currentDir=%cd%

if exist build\ (
	echo build folder exist, if you are using the USB for first time on this PC, this should be deleted!! 
	rmdir /s build ) 

echo Creating a new build folder...

mkdir build
chdir build

mkdir mak.mingw.x64
chdir mak.mingw.x64

echo Running CMake to generate Makefiles...
cmake -G "MinGW Makefiles" ../../
if %errorlevel% NEQ 0 (
    echo CMake generation failed. Press any key to exit.
    pause >nul
    goto end
)

echo Compiling with MinGW...
mingw32-make
if %errorlevel% NEQ 0 (
    echo Compilation failed. Press any key to exit.
    pause >nul
    goto end
) else ( goto end )

:error
echo !! An error was detected. Not success compilation - press key to continue.
pause >nul

:end
echo Success compilation !! - press key to continue.

cd %currentDir%\bin

