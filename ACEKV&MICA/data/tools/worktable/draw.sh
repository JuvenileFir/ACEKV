#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/50s50u/worktable/drawfor50s50u.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行
# 目前这里面是低log + 70% 目前看不出优势
