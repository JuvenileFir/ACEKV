cp ./mica/set.csv ./mica/valid.csv .
rename "s/^/mica_/" set.csv valid.csv
cp ./proto/set.csv ./proto/valid.csv .
rename "s/^/proto_/" set.csv valid.csv
mv mica*csv proto*csv ./tput_and_valid