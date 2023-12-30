time1=$(date +%y-%m-%d_%H-%M-%S)
#!/bin/bash
PY_DIR=/home/bwb/GPCode/proto-mica/data/tools/insert_test_latency.py
# 获取当前目录下所有文件名
file_names=$(ls /home/bwb/GPCode/proto-mica/data/ |grep -oP '.*(?=.log)')
full_file_names=$(ls /home/bwb/GPCode/proto-mica/data/ |grep '.log')

# 遍历文件名，并输出
# for file in $file_names; do
#   python3 /home/bwb/GPCode/proto-mica/data/insert_test.py $file
# done

# python3 $PY_DIR ${file_names[@]} #file_names[*]也行
for file in $full_file_names; do
    mv /home/bwb/GPCode/proto-mica/data/$file /home/bwb/GPCode/proto-mica/data/used_files
done
