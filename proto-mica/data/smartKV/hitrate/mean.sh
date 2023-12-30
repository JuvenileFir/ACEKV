#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/tools/ghr_last_mean.py

if [ $# == 1 ]
then
# WORK_DIR=$(cd $(dirname $0);pwd)
WORK_DIR=$(cd $1;pwd)
# 获取当前目录下所有文件名
file_names=$(ls -tr $WORK_DIR |grep -oP '.*(?=.log)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} $1 #file_names[*]也行
fi
