# rename "s/^/aLRU_1s_/" 06-15_14*  #通配符^表示匹配开头
# file_names=$(ls | grep -oP '.*(?=.*[mp][zu])')
file_names=$(ls | grep csv | grep -oP '.*mica_')
for file in $file_names; do
    rename -v "s/${file}//" *.csv
done
