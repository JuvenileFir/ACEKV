if [ $# == 2 ]
then
    if [ $1 == '-u' ]
    then
        echo "threshold in uniform" > u.log
        for i in 0 50 60 66 70 80 100   # h6l4
        # for i in 0 40 50 60 70 80 100   # h6l2
        # for i in 0 40 50 60 66 70 100   # h7l4
        do
            echo $i >> u.log
            ./mean.sh ${i}u >> u.log
        done
        # mv *u* $2
    fi

    if [ $1 == '-z' ]
    then
    echo "threshold in zipf" > z.log
        for i in 0 50 60 66 70 80 100   # h6l4
        # for i in 0 40 50 60 70 80 100   # h6l2
        # for i in 0 40 50 60 66 70 100   # h7l4
        do
            echo $i >> z.log
            ./mean.sh ${i}z >> z.log
        done
        # mv *z* $2
    fi
fi
if [[ $# == 1 && $1 == '-h' ]]
then
    echo "第一个参数为-u/-z，第二个参数表示移动到的文件目录"
fi