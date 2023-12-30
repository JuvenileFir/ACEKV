#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)

getratio='0.5'

threshold='50' #测cp要设100，跑命中率和性能要设50
zus=('u' 'z')
# zus=('u')# zus=('z')
logsizes=(30 31) #0 40 50 60 70 80 90 100)
wratios=(10 20 30 40 50 60 70 80 90 100) #按数据量对比ghr用
# wratios=(10 20 30 40 51 62 73 84 95)
###在server端运行，然后将参数传递到client端
    for zu in ${zus[@]}; do
    # if [ $zu == 'u' ]
    #     then
    #         # wratios=(10 20 30 40 50 60 70 80 92 100)
    #         # wratios=(100)
    #     else
    #         # wratios=(83 96 107)
    #         # wratios=(10 20 30 40 50 60 70 83 96 107)
    #         # wratios=(100)
    #     fi
    # for wratio in ${wratios[@]}; do
    # for logsize in ${logsizes[@]}; do
        # echo $wratio
        # echo "start executing on server"
        #echo ".perfectxhj621213."|nohup sudo -S ./server $thread > ./record/$filename 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}pz30_100_forcntcp2.log 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}_${logsize}_${wratio}.log 2>&1 &
        nohup sudo -S sudo ./main -f 2 -l 30 -t 30 -a 50 > ../data/${time1}p${zu}_log30_thres50_test.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r $getratio -t 8 -${zu} -l 8 -w 100
        exit
remotessh
        # sudo -S sudo ./client -n 6000000 -r $getratio -t 8 -$zu -l 8 -w $wratio
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
	    # done
    done
done
