# rename "s/^/aLRU_1s_/" 06-15_14*  #通配符^表示匹配开头
file_names=$(ls)
for file in $file_names; do
    cd ${file}
    # rename -v "s/^/${file}/" *
    ls
    cd ..
done
