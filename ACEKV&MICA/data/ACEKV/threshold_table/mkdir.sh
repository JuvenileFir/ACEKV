for i in 0 40 50 60 66 70 100
do
    echo $i
    mkdir ${i}z ${i}u
    mv ~/GPCode/proto-mica/data/11-06_21-24-27_pu_74thres${i}* ./${i}u
    mv ~/GPCode/proto-mica/data/11-06_21-24-27_pz_74thres${i}* ./${i}z
done