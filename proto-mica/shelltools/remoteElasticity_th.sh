#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)
zus=('u' 'z')
# zus=('u')
# zus=('z')
wratios='100'
# thresholds=(0 40 50 60 66 70 100) #跑threshold
# thresholds=(0 60 66 70 80 100) #跑thresholds
thresholds=(50) #跑tput
kvsizes=(8 64 512) # 8 64 512
times=(0 1 2) # 31) #0 40 50 60 70 80 90 100)
###在server端运行，然后将参数传递到client端

for kvsize in ${kvsizes[@]}; do
    if [ $kvsize == 8 ] #8-31 64-32 512-35 #各种large size的100%
    then
        logsize=31
    elif [ $kvsize == 64 ]
    then
        logsize=32
    elif [ $kvsize == 512 ]
    then
        logsize=35
    else
        echo "kvsize不匹配"
        exit
    fi
    for zu in ${zus[@]}; do
    for threshold in ${thresholds[@]}; do
    for idx in ${times[@]}; do
        echo "running acekv threshold: $threshold %"
        echo "start executing on server"
        nohup sudo -S sudo ./main_p${kvsize} -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}_${logsize}_kv${kvsize}_nr${idx}.log 2>&1 & #for tput

        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}_${logsize}_thres${threshold}.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r 0.5 -t 8 -$zu -l $kvsize -w $wratio
        exit
remotessh
        while true
        do
        serverpid=$(ps aux|grep "./main_p$kvsize"|grep -v "grep"|awk '{print $2}')
            if test -n "$serverpid"
            then
                echo ">>>>server is running"
                sleep 2
                for pid in ${serverpid[@]}; do
                echo $pid
                sudo kill $pid
                done
            else
                echo ">>>>no server"
                break
            fi
	    done
    done
    done
    done
done

for kvsize in ${kvsizes[@]}; do
    if [ $kvsize == 8 ] #8-31 64-32 512-35 #各种large size的100%
    then
        logsize=31
    elif [ $kvsize == 64 ]
    then
        logsize=32
    elif [ $kvsize == 512 ]
    then
        logsize=35
    else
        echo "kvsize不匹配"
        exit
    fi
    for zu in ${zus[@]}; do
    for idx in ${times[@]}; do
        echo "running mica"
        echo "start executing on server"
        nohup sudo -S sudo ./main_m${kvsize} -f 2 -l $logsize -t 30 > ../data/${time1}m${zu}_log${logsize}_kv${kvsize}_nr${idx}.log 2>&1 & #for tput
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r 0.5 -t 16 -$zu -l $kvsize -w $wratio
        exit
remotessh
        while true
        do
        serverpid=$(ps aux|grep "./main_m$kvsize"|grep -v "grep"|awk '{print $2}')
            if test -n "$serverpid"
            then
                echo ">>>>server is running"
                sleep 2
                for pid in ${serverpid[@]}; do
                echo $pid
                sudo kill $pid
                done
            else
                echo ">>>>no server"
                break
            fi
	    done
    done
    done
done