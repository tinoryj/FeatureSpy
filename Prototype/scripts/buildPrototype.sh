#!/bin/bash
if [ ! -d "build" ]; then
  mkdir build
else
  rm -rf build/*
fi
if [ ! -d "bin" ]; then
  mkdir bin
else
  rm -rf bin/*
fi
if [ ! -d "lib" ]; then
  mkdir lib
else
  rm -rf lib/*
fi

cd ./build
cmake .. -DSGX_HW=ON
make -j$(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)
cd ..
mkdir -p bin/Containers bin/Recipes
cp config.json ./bin
cp -r ./key ./bin/
cp ./lib/pow_enclave.signed.so ./bin
cp ./lib/km_enclave.signed.so ./bin