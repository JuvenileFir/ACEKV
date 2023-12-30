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
# labels = ["Baseline","Sorted-Batch","Unsorted-Batch","Unsorted-indiv"]

config = {
    "font.family":'serif',
    "font.size": 19,
    "mathtext.fontset":'stix',
    "font.serif": ['SimSun'],
}
rcParams.update(config)

def draw_insert_new(idx):

    font_sun = {
        "family":"SimSun",
        "weight":"normal",   # "bold"
        "size":19,
    }
    font_times = {
        "family":"Times New Roman",
        "weight":"normal",   # "bold"
        "size":19,
    }
    axTypeIndex = idx
    axType = [[1,2],[1,3],[0,1],[0,2],[0,3]]
    axtype = axType[axTypeIndex]
    BAR = False
    Xs = []
    Ys = []
    
    for csvfile in csvfiles:
        with open(csvfile,"r") as f:
            lines = f.read().split("\n")
            X = []
            Y = []
            # Y = []
            for line in lines:
                words = re.split(r"\t+", line)
                X.append(float(words[axtype[0]]))
                Y.append(float(words[axtype[1]]))
            Xs.append(X)
            Ys.append(Y)
       
    colors = ["red","blue","tab:purple","tab:blue","tab:green","tab:gray"]

    linestyles = ['-','-','-','-','-','-','-','-']
    plt.figure(figsize=(12.8,7.2))
    markers = [' ',' ',' ',' ',' ',' ',' ',' ']
    for i in range(0,len(Xs)):
        plt.plot(Xs[i],Ys[i],label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心

    xy_limit = [240000000, 100, 110, 3500000]
    print(plt.xlim(0,xy_limit[axtype[0]]))
    print(plt.ylim(0,xy_limit[axtype[1]]))
    # print(plt.ylim(50,xy_limit[axtype[1]]))
    if xy_limit[axtype[0]] == 200:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)
    elif xy_limit[axtype[0]] == 120000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],7)
    elif xy_limit[axtype[0]] == 160000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],9)
    elif xy_limit[axtype[0]] == 180000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],10)
    elif xy_limit[axtype[0]] == 200000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)
    elif xy_limit[axtype[0]] == 240000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],13)
    elif xy_limit[axtype[0]] == 320000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],9)
    elif xy_limit[axtype[0]] == 100:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)

    xy_name = [['0','20','40','60','80','100','120','140','160','180','200','220','240'],
            #    ['0','10','20','30','40','50','60','70','80'],
            #    ['0','40','80','120','160','200','240','280','320'],
               ['0','10','20','30','40','50','60','70','80','90','100'],
               ['0','10','20','30','40','50','60','70','80','90','100','110'],
            #    ['0','10','20','30','40','50'],
            #    ['50','60','70','80','90','100'],
               ['0','0.5','1.0','1.5','2.0','2.5','3.0','3.5']]#,'4.0','4.5','5.0','5.5','6.0']]
            #    ['0','0.1','0.2','0.3','0.4']]#,'3.0','3.5','4.0','4.5','5.0','5.5','6.0']]
    # y_ticks = np.linspace(0,500,20)
    if xy_limit[axtype[1]] == 100:
        # y_ticks = np.linspace(50,xy_limit[axtype[1]],6)
        y_ticks = np.linspace(0,xy_limit[axtype[1]],11)
    elif xy_limit[axtype[1]] == 110:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],12)
    elif xy_limit[axtype[1]] == 50:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],6)
    elif xy_limit[axtype[1]] == 150000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],4)
    elif xy_limit[axtype[1]] == 3500000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],8)
    elif xy_limit[axtype[1]] == 2500000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],6)
    elif xy_limit[axtype[1]] == 3000000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],7)


    plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=19) #刻度
    plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=19)

    ax = plt.gca()
    xylabels = ['写入请求数','索引装载率','读取命中率','新写入吞吐量']
    xylabel_units = ['M','\%','\%','Mops']
    
    # ax.set_title('$\mathrm{proto\ &\ mica*}$的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    # ax.set_title('$\mathrm{mica*}$的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    ax.set_title('$\mathrm{mica*}$的各类型吞吐量随' + xylabels[axtype[0]] + '的变化趋势')
    ax.set_xlabel(xylabels[axtype[0]] + ' $\mathrm{(' + xylabel_units[axtype[0]] + ')}$') 
    ax.set_ylabel(xylabels[axtype[1]] + ' $\mathrm{(' + xylabel_units[axtype[1]] + ')}$')
    # ax.invert_yaxis()
    # ax.legend(prop=font_times, loc = 0, ncol = 2) # 图例
    ax.legend(prop=font_times) # 图例

    plt.savefig(csvdir + '1' + str(idx) + "_mt.svg", format = "svg")
    plt.savefig(csvdir + '1' + str(idx) + "_mt.jpg")
    plt.show()


files = []
logfiles = []
csvfiles = []
labels = []
# drawFuncIdx = [0,1,2,3,4]
drawFuncIdx = [1,4]
for argva in sys.argv[2:]:
    # logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")
    labels.append(argva)


# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

for idx in drawFuncIdx:
    draw_insert_new(idx)

# draw_insert()
# draw()
# print(X,Y)

