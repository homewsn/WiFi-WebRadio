#!/bin/bash

echo "menuconfig.sh 14.02.2024"
echo ""

. $IDF_PATH/export.sh
idf.py set-target esp32
idf.py menuconfig