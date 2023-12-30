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
    Ys = []

    for csvfile in csvfiles:
        with open(csvfile,"r") as f:
            lines = f.read().split("\n")
            # X = []
            Y = []
            # Y = []
            for line in lines:
                words = re.split(r"\t+", line)
                # X.append(float(words[axtype[0]]))
                Y.append(float(words[2]))
            # Xs.append(X)
            Ys.append(Y)

    colors = ["tab:blue","tab:orange","tab:blue","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]

    width = 0.4 # 柱子的宽度
    # Pts = np.arange(1,21,2)
    Pts = np.arange(1,21,2)
    linestyles = [':',':',':',':',':',':',':',':','-']
    plt.figure(figsize=(15,12))
    markers = ['s','^','o','x','D','v',' ',' ',' ']
    for i in range(0,len(Ys)):
        plt.bar(Pts - 0.2, Ys[i], width, label=labels[i], color=colors[i])#rfacecolor='white'可画空心

    # 计算每个柱子在x轴上的位置，保证x轴刻度标签居中
    # plt.ylabel('Scores')
    # x轴刻度标签位置不进行计算
    plt.xlim(0,18)
    plt.ylim(0,10000000)
    x_ticks = np.linspace(1,17,9)
    y_ticks = np.linspace(0,10000000,11)
    x_labels=['10','20','30','40','50','60','70','80','90']
    y_labels=['0','1.0','2.0','3.0','4.0','5.0','6.0','7.0','8.0','9.0','10.0']
    plt.xticks(x_ticks,x_labels,fontsize=60) #刻度
    plt.yticks(y_ticks,y_labels,fontsize=60) #刻度

    # plt.xticks(Pts, labels=x_labels)
    # plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=40)

    plt.legend()

    # plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=40) #刻度
    # plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=40)

    ax = plt.gca()
    xylabel_units = ['M','\%','\%','M']
    ax.set_xlabel('$\mathrm{Load\ Factor\ (' + xylabel_units[1] + ')}$') 
    ax.set_ylabel('$\mathrm{Total\ Cuckoo\ Path(' + xylabel_units[3] + ')}$')
    ax.legend(prop=font_times, bbox_to_anchor=(0.48,-0.12),loc = 9, ncol = 6, frameon=False) # 图例
    plt.subplots_adjust(left=0.2, right=0.97, top=0.97, bottom=0.22)

    plt.savefig(csvdir + "thresu_bar" + ".svg", format = "svg")
    plt.savefig(csvdir + "thresu_bar" + "_image.jpg")
    plt.show()

files = []
logfiles = []
csvfiles = []
labels = []
# drawFuncIdx = [0,1,2,3,4]
# drawFuncIdx = [0,1,2,3,4]

for argva in sys.argv[2:]:
    # logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")
    print(argva)
    argva = re.sub('mz',"MICA-t_zipf",argva)
    argva = re.sub('mu',"MICA-t_uniform",argva)
    argva = re.sub('pz',"CaleKV_zipf",argva)
    argva = re.sub('pu',"CaleKV_uniform",argva)
    argva = re.sub('cz',"MemC3-t_uniform",argva)
    argva = re.sub('cu',"MemC3-t_uniform",argva)
    argva = re.sub('d',"delay",argva)
    # print(argva)
    res = re.findall('[0-9]+',argva)
    sep = " + "
    resstr = sep.join(res)
    print(resstr)
    # labels.append(res[-1] + '%') 
    labels.append("ACEKV-cuckoo") 
    # labels.append(argva)


# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:

draw_insert_new()



# draw_insert()
# draw()
# print(X,Y)

