#!/usr/bin/env bash
cp -r /mnt/c/Users/yp636/Documents/workdir/6.7.11apidemo ~/workdir/
cd ./build
rm -rf *
cmake ..
make -j
sudo ./testprogram
