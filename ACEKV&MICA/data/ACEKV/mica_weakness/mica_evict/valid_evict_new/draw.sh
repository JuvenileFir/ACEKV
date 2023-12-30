#!/bin/bash
WORK_DIR=$(cd $(dirname $0);pwd)
PY_DIR=$(ls |grep drawfor2.py)
# PY_DIR=$(ls |grep drawfor2.py)

file_names=$(ls -t $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} 1 ${file_names[@]}