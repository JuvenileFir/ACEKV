#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/5s95u/valid_reload/drawfor5s50uvalid.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls -tr $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行

#valid-8 val size-70% trigger-1G
#在uniform下，装载率的差异就可以反映在命中率上了