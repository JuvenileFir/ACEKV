#!/bin/bash
PY_DIR=/home/bwb/GPCode/Memc3/data/memc3/50s50u/drawfor50s50u200s.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行
