#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S_)

getratio='0.5'

# threshold='50' #测cp要设100，跑命中率和性能要设50
zus=('u' 'z')
# zus=('u')
# zus=('z')
# logsizes=(30 31)
# wratios=(10 20 30 40 50 60 70 80 90 100) #按数据量对比ghr用
# wratios=(70 80 90) #按数据量对比ghr用
wratios=(100) #tput直接用100
# wratios=(110) #对比cp用
# wratios=(120) #测load factor用
thresholds=(0 40 50 60 70 80 90 100) #跑thresholds
kvsizes=(8) # 8 64 256 512
times=(0 1 2) # 31) #0 40 50 60 70 80 90 100)
# times=(0)
###在server端运行，然后将参数传递到client端

for kvsize in ${kvsizes[@]}; do
    if [ $kvsize == 8 ] #8-31 64-32 512-35 #各种large size的100%
        then
            logsize=31
        elif [ $kvsize == 64 ]
        then
            logsize=32
        elif [ $kvsize == 256 ]
        then
            logsize=33
        elif [ $kvsize == 512 ]
        then
            logsize=33 #应当是34
        else
            echo "kvsize不匹配"
            exit
    fi
    for zu in ${zus[@]}; do
    for threshold in ${thresholds[@]}; do
    for wratio in ${wratios[@]}; do
    for idx in ${times[@]}; do
        echo $wratio
        echo "start executing on server"
        #echo ".perfectxhj621213."|nohup sudo -S ./server $thread > ./record/$filename 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}pz30_100_forcntcp2.log 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}_${logsize}_${wratio}load.log 2>&1 &
        nohup sudo -S sudo ./main_p${kvsize} -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}_log${logsize}_kv${kvsize}_thres${threshold}_tput${idx}.log 2>&1 &
        # nohup sudo -S sudo ./main -f 2 -l $logsize -t 30 -a $threshold > ../data/${time1}p${zu}${logsize}_thres${threshold}cp.log 2>&1 &
        ssh bwb@10.176.64.35 << remotessh
        cd ./Client/build/app/
        sudo -S sudo ./client -n 6000000 -r $getratio -t 8 -$zu -l $kvsize -w $wratio
        exit
remotessh
        # sudo -S sudo ./client -n 6000000 -r 0.95 -t 8 -$zu -l 8 -w $wratio

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
done