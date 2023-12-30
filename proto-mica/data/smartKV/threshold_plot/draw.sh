#!/bin/bash
WORK_DIR=$(cd $(dirname $0);pwd)
PY_DIR=$(ls |grep draw*py)


file_names=$(ls -tr $WORK_DIR/tput_res_files |grep -oP '.*x(?=.csv)')
python3 $PY_DIR ${WORK_DIR}/tput_res_files ${file_names[@]} # 0
        