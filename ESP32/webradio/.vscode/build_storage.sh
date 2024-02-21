#!/bin/bash

echo "build_storage.sh 14.02.2024"
echo ""

CWD="$(pwd)"
cp build/config/sdkconfig.h /opt/mkspiffs/include
cd /opt/mkspiffs
make clean
make
cd $CWD
/opt/mkspiffs/mkspiffs -c storage -s 0x20000 build/storage.bin