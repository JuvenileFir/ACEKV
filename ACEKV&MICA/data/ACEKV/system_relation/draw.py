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
csvdir = sys.argv[1] + "/"
nameIdx = [1,3,0,2,4]

# labels = ["Baseline","Sorted-Batch","Unsorted-Batch","Unsorted-indiv"]

config = {
    "font.family":'serif',
    "font.size": 50,
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
        "size":43,
}


def draw_insert_new():

    
    labels = ["ACEKV","MICA","MemC3","d"]
    colors = ["red","blue","tab:purple","tab:orange","tab:blue","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]
    linestyles = ['-','--','-.','-','-','-','-','-','-']
    plt.figure(figsize=(15,12))
    markers = ['^','o','s',' ',' ',' ',' ',' ',' ']

    # Xs = [[61.4362],[60.8000],[63.8290]]
    Ys = np.array([3936223,4751333,540824])
    # Ys = [[3936223],[4751333],[540824]]
    Xs = np.array([61.4362,60.8000,63.8290])
    for i in range(0,len(Xs)):
        # plt.plot(Xs[i],Ys[i],linewidth =4.0,label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心
        plt.scatter(Xs[i],Ys[i],s=300,label=labels[i],marker=markers[i],facecolors='none',edgecolors=colors[i],linewidths=5) # markerfacecolor='white'可画空心
    
    
    # plt.scatter(x,y)
    print(plt.xlim(55,65))
    print(plt.ylim(-100000,5000000))
    x_ticks = np.linspace(55,65,3)
    y_ticks = np.linspace(0,5000000,11)
    # xy_name = [,]

    plt.xticks(x_ticks,['55','60','65'],fontsize=40)
    plt.yticks(y_ticks,['0','0.5','1.0','1.5','2.0','2.5','3.0','3.5','4.0','4.5','5.0'],fontsize=40) #刻度

    ax = plt.gca()
    xylabels = ['请求数','索引装载率','读取命中率','吞吐量']
    xylabel_units = ['M','\%','\%','Mops']
    
    # ax.set_xlabel('Insert Data %',font)
    # ax.set_title("protoKV vs mica*的命中率变化趋势" ,font)
    # ax.set_title('$\mathrm{proto\ &\ mica*}$的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    # ax.set_title('不同哈希算法的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    # ax.set_title('不同失效检查开启阈值的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    # ax.set_title('$\mathrm{mica*}$的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    ax.set_xlabel('$\mathrm{Hit\ Rate( \% )}$')
    ax.set_ylabel('$\mathrm{Throughput\ ( Mops )}$') 
    # ax.invert_yaxis()
    ax.legend(prop=font_times, bbox_to_anchor=(0.45,-0.12),loc = 9, ncol = 3, frameon=False) # 图例
    plt.subplots_adjust(left=0.12, right=0.97, top=0.98, bottom=0.2)

    plt.savefig(csvdir + "thr.svg", format = "svg")
    plt.savefig(csvdir + "thr_image.jpg")
    plt.show()

files = []
logfiles = []
csvfiles = []
labels = []

draw_insert_new()

# draw_insert()
# draw()
# print(X,Y)

