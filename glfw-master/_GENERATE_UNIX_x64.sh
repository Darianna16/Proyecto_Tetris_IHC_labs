#!/bin/bash

currentDir=$(pwd)

if [ -d "build" ]; then
    echo "La carpeta build existe. Si es la primera vez que usas este USB en esta PC, ¡debería eliminarse!"
    rm -rf build
fi

echo "Creando una nueva carpeta build..."

mkdir -p build
cd build || exit 1

mkdir -p mak.gcc.x64
cd mak.gcc.x64 || exit 1

echo "Ejecutando CMake para generar los Makefiles..."
cmake -G "Unix Makefiles" ../../
if [ $? -ne 0 ]; then
    echo "Falló la generación de CMake. Presiona una tecla para salir."
    read -n 1 -s
    cd "$currentDir/bin" 2>/dev/null
    exit 1
fi

echo "Compilando con GCC/Make..."
make
if [ $? -ne 0 ]; then
    echo "La compilación falló. Presiona una tecla para salir."
    read -n 1 -s
    cd "$currentDir/bin" 2>/dev/null
    exit 1
fi

echo "¡Compilación exitosa! - presiona una tecla para continuar."
read -n 1 -s

cd "$currentDir/bin" || exit 1