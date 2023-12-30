time1=$(date +%y-%m-%d_%H-%M-%S)
#!/bin/bash
# 获取当前目录下所有文件名
file_names=$(ls /home/bwb/GPCode/Memc3/data/ |grep -oP '.*(?=.log)')
full_file_names=$(ls /home/bwb/GPCode/Memc3/data/ |grep '.log')

# 遍历文件名，并输出
# for file in $file_names; do
#   python3 /home/bwb/GPCode/Memc3/data/insert_test.py $file
# done

python3 /home/bwb/GPCode/Memc3/data/insert_test.py ${file_names[@]} #file_names[*]也行
for file in $full_file_names; do
    mv /home/bwb/GPCode/Memc3/data/$file /home/bwb/GPCode/Memc3/data/used_files
done
