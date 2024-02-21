#!/bin/bash

echo "flash.sh 14.02.2024"
echo ""

. $IDF_PATH/export.sh
idf.py -p $IDF_PORT flash