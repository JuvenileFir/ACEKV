#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/kvsize/drawforkvsizep.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls -tr $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行

# 1024 proto 16G
# 128  proto 16G
# 256  proto 16G
# 512  proto 16G
# 64   proto 4G
