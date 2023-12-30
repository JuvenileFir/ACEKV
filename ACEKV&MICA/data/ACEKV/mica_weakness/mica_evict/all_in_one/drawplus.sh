#!/bin/bash
WORK_DIR=$(cd $(dirname $0);pwd)
PY_DIR=$(ls |grep drawfor2.py)
REFINE_DIR=/home/bwb/ACEKV/tools/insert_mica_evict.py

lf_res=0.1
sq_res=1000000

if [ $# == 1 ]
then
    if [ $1 == "-h" ]
    then
        echo 将执行hit_rate相关流程
        # 获取当前目录下所有文件名
        logfile_names=$(ls $WORK_DIR |grep -oP '.*(?=.log)')
        python3 $REFINE_DIR 2 $WORK_DIR ${logfile_names[@]} $hit_lf_res $hit_sq_res
        # 获取当前目录下所有文件名
        file_names=$(ls $WORK_DIR |grep -oP '.*a(?=.csv)')
        python3 $PY_DIR ${WORK_DIR} 2 ${file_names[@]} # 0
        file_names=$(ls $WORK_DIR |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR ${WORK_DIR} 0 ${file_names[@]} # 1
        file_names=$(ls $WORK_DIR |grep -oP '.*t(?=.csv)')
        python3 $PY_DIR ${WORK_DIR} 3 ${file_names[@]} # 2
        file_names=$(ls $WORK_DIR |grep '.csv')
        for file in $file_names; do
            mv $file ./hit_res_files/
        done
    elif [ $1 == "-t" ]
    then
        echo 将执行throughput相关流程
        logfile_names=$(ls $WORK_DIR/tputlog |grep -oP '.*(?=.log)')
        python3 $REFINE_DIR 3 $WORK_DIR ${logfile_names[@]} $tput_lf_res $tput_sq_res

        file_names=$(ls $WORK_DIR |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR ${WORK_DIR} 1 ${file_names[@]} # 3
        file_names=$(ls $WORK_DIR |grep -oP '.*t(?=.csv)')
        python3 $PY_DIR ${WORK_DIR} 4 ${file_names[@]} # 4
        file_names=$(ls $WORK_DIR |grep '.csv')
        for file in $file_names; do
            mv $file ./tput_res_files/
        done
    elif [ $1 == "-0" ]
    then
        echo 将重画图0
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*a(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 2 ${file_names[@]} # 0
    elif [ $1 == "-1" ]
    then
        echo 将重画图1
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 0 ${file_names[@]} # 1
        echo 将重画图1效果有吗

    elif [ $1 == "-2" ]
    then
        echo 将重画图2
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*t(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 3 ${file_names[@]} # 2
    elif [ $1 == "-3" ]
    then
        echo 将重画图3
        file_names=$(ls $WORK_DIR/tput_res_files |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR $WORK_DIR/tput_res_files 1 ${file_names[@]} # 3
    elif [ $1 == "-4" ]
    then
        echo 将重画图4
        file_names=$(ls $WORK_DIR/tput_res_files |grep -oP '.*t(?=.csv)')
        python3 $PY_DIR $WORK_DIR/tput_res_files 4 ${file_names[@]} # 4
    elif [ $1 == "-a" ]
    then
        echo 将重画图0-3
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*a(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 2 ${file_names[@]} # 0
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 0 ${file_names[@]} # 1
        file_names=$(ls $WORK_DIR/hit_res_files |grep -oP '.*t(?=.csv)')
        python3 $PY_DIR $WORK_DIR/hit_res_files 3 ${file_names[@]} # 2
        file_names=$(ls $WORK_DIR/tput_res_files |grep -oP '.*x(?=.csv)')
        python3 $PY_DIR $WORK_DIR/tput_res_files 1 ${file_names[@]} # 3   
    elif [ $1 == "clean" ]
    then
        rm -f ./res_files/* 
    else
        echo 请输入正确参数
    fi
else
    # 获取当前目录下所有文件名
    # logfile_names=$(ls $WORK_DIR |grep -oP '.*(?=.log)')
    # python3 $REFINE_DIR  $WORK_DIR ${logfile_names[@]} $lf_res $sq_res
    # 获取当前目录下所有文件名

    file_names=$(ls $WORK_DIR |grep -oP '.*x[a-z]+(?=.csv)')
    python3 $PY_DIR ${WORK_DIR} 0 ${file_names[@]}  #t
    file_names=$(ls $WORK_DIR |grep -oP '.*t[a-z]+(?=.csv)')
    python3 $PY_DIR ${WORK_DIR} 1 ${file_names[@]}  #x

    # file_names=$(ls $WORK_DIR |grep '.csv')
    # for file in $file_names; do
    #     mv $file ./res_files/
    # done

fi
