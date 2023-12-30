#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)
zus=('u' 'z')
# zus=('u')
# zus=('z')
# wratios=(10 20 30 40 50 60 70 80 90 100)
wratios=(100)
# logsizes=(30 31)
logsizes=(33 34 36)
hashsizes=(25)
###在server端运行，然后将参数传递到client端
    for logsize in ${logsizes[@]}; do
    for hashsize in ${hashsizes[@]}; do
    for zu in ${zus[@]}; do
    if [ $logsize == 33 ]
    then
        kvsize=8
    elif [ $logsize == 34 ]
    then
        kvsize=64
    elif [ $logsize == 36 ]
    then
        kvsize=512
    else
        echo "kvsize不匹配"
        exit
    fi
        for wratio in ${wratios[@]}; do
        echo $wratio
        echo "start executing on server"
        nohup sudo -S sudo ./main -f 2 -l $logsize -t $hashsize > ../data/${time1}p${zu}_tput${kvsize}.log 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t $hashsize > ../data/${time1}c${zu}${logsize}_${wratio}ghr.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r 0.5 -t 8 -$zu -l $kvsize -w $wratio
        exit
remotessh

        while true
        do
        serverpid=$(ps aux|grep "./main"|grep -v "grep"|awk '{print $2}')
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