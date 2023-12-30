#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)
zus=('u' 'z')
# zus=('u')
# zus=('z')
# wratios=(10 20 30 40 51 62 73 84 95)
# wratios=(80 90)
# wratios=(10)
hashsizes=(25) #跑lf应当是4个线程25的hashsize
logsizes=(33) #单线程31，4线程33
wratios=(100)
###在server端运行，然后将参数传递到client端
    for hashsize in ${hashsizes[@]}; do
    for logsize in ${logsizes[@]}; do
    for wratio in ${wratios[@]}; do
    for zu in ${zus[@]}; do
        echo $wratio
        echo "start executing on server"
        #可以考虑区分reload和no reload
        nohup sudo -S sudo ./main -f 2 -l $logsize -t $hashsize > ../data/${time1}c${zu}_lf.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r 0.5 -t 8 -$zu -l 8 -w $wratio
        exit
remotessh
        # sudo -S sudo ./client -n 6000000 -r 0.95 -t 8 -$zu -l 8 -w $wratio

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