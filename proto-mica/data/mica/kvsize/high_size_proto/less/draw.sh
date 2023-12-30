#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/kvsize/drawforkvsizep.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls -t $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行

# 1024proto8G
# 128proto2G
# 256proto4G
# 512proto8G
# 64proto2G
