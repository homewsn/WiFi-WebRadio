#!/bin/bash

echo "flash_storage.sh 14.02.2024"
echo ""

$IDF_TOOLS_PATH/python_env/idf5.1_py3.10_env/bin/python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port $IDF_PORT --baud 921600 write_flash 0x110000 build/storage.bin
# or
# $IDF_TOOLS_PATH/python_env/idf5.1_py3.10_env/bin/python $IDF_PATH/components/partition_table/parttool.py --port "$IDF_PORT" write_partition --partition-name=storage --input "build/storage.bin"
