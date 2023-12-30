#!/bin/bash
PY_DIR=mean.py


WORK_DIR=$(cd $(dirname $0);pwd)

# 获取当前目录下所有文件名
file_names=$(ls $WORK_DIR |grep -oP '.*(?=.csv)')
echo ${file_names[0]}
python3 $PY_DIR ${WORK_DIR} ${file_names[@]}

