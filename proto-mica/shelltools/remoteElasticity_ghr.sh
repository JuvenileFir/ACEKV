#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)

getratio='0.5'

threshold='50' #测cp要设100，跑命中率和性能要设50
zus=('u' 'z')
# zus=('z')
# logsizes=(30 31)
logsizes=(31)
wratios=(10 20 30 40 50 60 70 80 90 100) #按数据量对比ghr用
times=(0 1 2)
# times=(0)
###在server端运行，然后将参数传递到client端
    for zu in ${zus[@]}; do
    for wratio in ${wratios[@]}; do
    for logsize in ${logsizes[@]}; do
    for idx in ${times[@]}; do
        echo $wratio
        echo "start executing on server"
        nohup sudo -S sudo ./main_p -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}${logsize}_${wratio}_ghr${idx}.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r $getratio -t 8 -$zu -l 8 -w $wratio
        exit
remotessh

        while true
        do
        serverpid=$(ps aux|grep "./main_p"|grep -v "grep"|awk '{print $2}')
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

    for zu in ${zus[@]}; do
    for wratio in ${wratios[@]}; do
    for logsize in ${logsizes[@]}; do
    for idx in ${times[@]}; do
        echo $wratio
        echo "start executing on server"
        nohup sudo -S sudo ./main_m -f 2 -l $logsize -t 30 > ../data/${time1}m${zu}${logsize}_${wratio}_ghr${idx}.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r $getratio -t 16 -$zu -l 8 -w $wratio
        exit
remotessh

        while true
        do
        serverpid=$(ps aux|grep "./main_m"|grep -v "grep"|awk '{print $2}')
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
