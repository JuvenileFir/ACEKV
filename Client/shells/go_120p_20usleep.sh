#!/bin/bash
time1=$(date +%m-%d_%H-%M-%S)

# sudo ./build/app/client 0.99 # > data/${time1}.log #200%
# sudo ./build/app/client -n 10000000 # > data/${time1}.log #200%
client='/home/bwb/Client/build/app/client_120p_d20u'
num='6000000' #6000000 * 20 --- 58720256是200%
#128  --- 9786709  --- 489225
#256  --- 14680064 --- 734003
#512  --- 29360128 --- 1468006
#512(2-2)  --- 29360128 --- 1468006
#1024 --- 58720256 --- 2936012

ratio='0.5'
kvsize='512' #tiny-8 small-64 big-1024   
# 8:16:32:40:48:56:64:128:256:512:1024
if [ $1 == "mu" ]
then
    echo "发送mica  uniform  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 16 -u -l $kvsize
elif [ $1 == "mz80" ]
then
    echo "发送mica  zipf_theta:0.8  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 16 -z 0.8 -l $kvsize
elif [ $1 == "mz" ]
then
    echo "发送mica  zipf_theta:0.99  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 16 -l $kvsize
elif [ $1 == "pu" ]
then
    echo "发送proto  uniform  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 8 -u -l $kvsize
elif [ $1 == "pz80" ]
then
    echo "发送proto  zipf_theta:0.8  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 8 -z 0.8 -l $kvsize
elif [ $1 == "pz" ]
then
    echo "发送proto  zipf_theta:0.99  get_ratio:$ratio kv size:$kvsize"
    sudo $client -n $num -r $ratio -t 8 -l $kvsize
else
    echo 请输入正确格式
fi

# sudo ./client -n 6000000 -r 0.95 -t 8
# sudo ./client -n 6000000 -r 0.95 -t 8 -z 0.8
# sudo ./client -n 6000000 -r 0.95 -t 16
