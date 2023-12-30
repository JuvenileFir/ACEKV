#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/mica/5s95z/valid_reload/drawfor5s50zvalid.py
WORK_DIR=$(cd $(dirname $0);pwd)
# 获取当前目录下所有文件名
file_names=$(ls -tr $WORK_DIR |grep -oP '.*(?=.csv)')
python3 $PY_DIR ${WORK_DIR} ${file_names[@]} #file_names[*]也行

#valid-8 val size-70% trigger-1G
#这里proto比mica装载率更高，但是命中率相当，是因为zipf-get只看一小部分数据