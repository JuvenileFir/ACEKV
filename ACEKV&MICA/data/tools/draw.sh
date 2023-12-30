BASIC_DIR='/home/bwb/GPCode/proto-mica/data/mica/'
xx=(
    # 5s95u/100p_expand
    # 5s95u/200p_fixedSet
    # 5s95z/200p_fixedSet/zipf99
    # 5s95z/200p_fixedSet/zipf80
    # 5s95z/100p_expand/zipf99
    # 5s95z/100p_expand/zipf80

    # 50s50z/fixedR/zipf99
    # 50s50z/fixedR/zipf80
    # 50s50z/matchR/zipf99
    # 50s50z/matchR/zipf80

    # 50s50u/99
    # 50s50u/96

    # 50z50z
    # 50u50u

    # 95s5z/200p_fixedset/zipf99
    # 95s5z/200p_fixedset/zipf80
    # 95s5z/100p_expand/zipf99
    # 95s5z/100p_expand/zipf80 

    # 95s5u/200p_fixedset
    # 95s5u/100p_expand

    # # 100percent/zipf
    # # 100percent/uniform

    # kvsize/fixed_mica
    # kvsize/low_size_mica
    # kvsize/low_size_proto
    # kvsize/rand_ratio_proto
    # kvsize/step_fixed_proto/less
    # kvsize/step_fixed_proto/more
    # kvsize/compare/8
    # kvsize/compare/16
    # kvsize/compare/32
    # kvsize/compare/40
    # kvsize/compare/48
    # kvsize/compare/56
    # kvsize/compare/64
    # kvsize/compare/128
    # kvsize/compare/256
    # kvsize/compare/512
    # kvsize/compare/1024

    # 100percent/uniform
    # 100percent/zipf

    log_change
)
for dir in ${xx[@]};do
    ${BASIC_DIR}${dir}/draw.sh
done
