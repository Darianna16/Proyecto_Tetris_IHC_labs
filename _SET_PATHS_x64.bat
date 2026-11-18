
rem Backup the current PATH to a file
echo Backing up the current PATH...
echo %path%>path_backup.txt

rem Store the base directory
set baseDir=%cd%

rem Display the current file path for the compiler and CMake
echo The current file path will use to our compiler and cmake is:
echo %baseDir%

rem Define directories for the compiler and CMake
set compilerDir=%baseDir%\MinGW\mingw162_64\bin
set cmakeDir=%baseDir%\Cmake\cmake-4.4.3-windows-x86_64\bin

rem Display the defined paths
echo "compilerDir = " %compilerDir%
echo "cmakeDir = " %cmakeDir%

rem Update the PATH environment variable
echo Updating the PATH environment variable...
set path=%compilerDir%;%PATH%
set path=%cmakeDir%;%PATH%

::cd %currentDir%
