import os
import re
import sys
# from tkinter import BOTTOM
# import matplotlib
# matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib import rcParams
import matplotlib.image as mpimg
import numpy as np
logdir = "/home/bwb/GPCode/proto-mica/data/"
# csvdir = logdir + "mica/"
# csvdir = logdir + "mica/50s50z/fixedR/zipf80/"
nameIdx = [1,3,0,2,4]

# labels = ["Baseline","Sorted-Batch","Unsorted-Batch","Unsorted-indiv"]

config = {
    "font.family":'Times New Roman',
    "font.size": 70,
    "mathtext.fontset":'stix',
    "font.serif": ['SimSun'],
}
rcParams.update(config)
font_sun = {
        "family":"SimSun",
        "weight":"normal",   # "bold"
        "size":19,
}
font_times = {
        "family":"Times New Roman",
        "weight":"bold",   # "bold"
        "size":63,
}


def draw_insert_new():
    Ys = [[693505.377,730782.052],[748283.431,802753.342]]

    colors = ["#f05326","#3682be","tab:purple","tab:orange","tab:blue","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]

    width = 0.3 # 柱子的宽度
    Pts = np.linspace(1,2.5,2)
    linestyles = [':',':',':',':',':',':',':',':','-']
    plt.figure(figsize=(15,12))
    markers = ['s','^','o','x','D','v',' ',' ',' ']
    hatches = [':','/']
    comp = [-0.15, 0.15]
    for i in range(0,len(Ys)):
        plt.bar(Pts+comp[i], Ys[i], width, label=labels[i], color=colors[i], hatch=hatches[i])#rfacecolor='white'可画空心

    # 计算每个柱子在x轴上的位置，保证x轴刻度标签居中
    # plt.ylabel('Scores')
    # x轴刻度标签位置不进行计算
    plt.xlim(0,3.5)
    plt.ylim(0,1000000)
    x_ticks = np.linspace(1,2.5,2)
    y_ticks = np.linspace(0,1000000,6)
    x_labels=['Uniform','Skewed']
    y_labels=['0.0','0.2','0.4','0.6','0.8','1.0']
    plt.xticks(x_ticks,x_labels,fontsize=60) #刻度
    plt.yticks(y_ticks,y_labels,fontsize=60) #刻度

    # plt.xticks(Pts, labels=x_labels)
    # plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=40)

    plt.legend()

    # plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=40) #刻度
    # plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=40)

    ax = plt.gca()
    xylabel_units = ['M','\%','\%','Mops']
    # ax.set_xlabel('$\mathrm{Load\ Factor\ (' + xylabel_units[1] + ')}$') 
    ax.set_ylabel('$\mathrm{Throughput\ (' + xylabel_units[3] + ')}$')
    ax.legend(prop=font_times, bbox_to_anchor=(0.48,-0.05),loc = 9, ncol = 1, frameon=False) # 图例
    plt.subplots_adjust(left=0.15, right=0.97, top=0.97, bottom=0.28)

    plt.savefig("mweakness_bar" + ".svg", format = "svg")
    plt.savefig("mweakness_bar" + "_image.jpg")
    plt.show()

files = []
logfiles = []
csvfiles = []
labels = ['w/ Hash Deletion','w/o Hash Deletion']
# drawFuncIdx = [0,1,2,3,4]
# drawFuncIdx = [0,1,2,3,4]

# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:

draw_insert_new()



# draw_insert()
# draw()
# print(X,Y)

