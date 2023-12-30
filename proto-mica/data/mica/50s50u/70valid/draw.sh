#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/50s50u/70valid/drawfor50s50z70valid.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls -tr $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行
# 目前这里面是低log + 70% 目前看不出优势
