import os
import re
import sys
# from tkinter import BOTTOM
import matplotlib
matplotlib.use('Agg')
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
    "font.family":'SimSun',
    "font.size": 60,
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
        "size":45,
}


def draw_insert_new(idx):

    
    axTypeIndex = idx
    axType = [[0,2],[1,2]]
        
    axtype = axType[axTypeIndex]
    BAR = False
    Xs = []
    Ys = []
    Xsb = []
    Ysb = []
    
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
    for validcsv in validfiles:
        with open(validcsv,"r") as f:
            lines = f.read().split("\n")
            X = []
            Y = []
            # Y = []
            for line in lines:
                words = re.split(r"\t+", line)
                X.append(float(words[1]))
                Y.append(float(words[2]))
            Xsb.append(X)
            Ysb.append(Y)
       
    # newYs = []
    # for Y in Ys:
        # Y = transformFromMsToS(Y)
        # newYs.append(Y)
    # Ys = newYs
    # print(Ys)
    colors = ["red","blue","tab:purple","tab:orange","tab:blue","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]

   
    linestyles = ['-','--','-.',':',':',':','-','-','-']
    fig, ax = plt.subplots(figsize=(15,12))
    bx = ax.twinx()
    markers = [' ',' ',' ',' ',' ',' ',' ',' ',' ']
    # lines = bx.plot(Xsb[0],Ysb[0],linewidth =4.0,label=labels[3],linestyle=linestyles[3],marker=markers[3],markersize=8,color=colors[3]) # markerfacecolor='white'可画空心
    # lines = lines + ax.plot(Xs[0],Ys[0],linewidth =4.0,label=labels[0],linestyle=linestyles[0],marker=markers[0],markersize=8,color=colors[0]) # markerfacecolor='white'可画空心
    lines = []
    for i in range(0,len(Xsb)):
        lines = lines + bx.plot(Xsb[i],Ysb[i],linewidth =4.0,label=labels[i+3],linestyle=linestyles[i+3],marker=markers[i+3],markersize=8,color=colors[i+3]) # markerfacecolor='white'可画空心
        lines = lines + ax.plot(Xs[i],Ys[i],linewidth =4.0,label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心

    xy_limit = [300000000, 100, 82]
    ax.set_xlim(-5,105)
    ax.set_ylim(-2,82)
    bx.set_ylim(-2,102)

    ax.set_xlabel(' $\mathrm{Load\ Factor\ (\%)}$') 
    ax.set_ylabel(' $\mathrm{Evict\ Ratio\ (\%)}$')
    bx.set_ylabel(' $\mathrm{Valid\ Ratio\ (\%)}$')
    
    # print(plt.xlim(0,xy_limit[axtype[0]]))
    # print(plt.ylim(0,xy_limit[axtype[1]]))  
    if xy_limit[axtype[0]] == 200:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)
    elif xy_limit[axtype[0]] == 300000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],7)
    elif xy_limit[axtype[0]] == 100:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)

    xy_name = [['0','50','100','150','200','250','300'],#'350','400'],
            #    ['0','20','40','60','80','100','120','140','160','180','200'],
            #    ['0','10','20','30','40','50','60','70','80','90','100','110','120','130','140','150'],
               ['0','10','20','30','40','50','60','70','80','90','100'],
               ['0','10','20','30','40','50','60','70','80']]
            #    ['0','10','20','30','40','50','60','70','80','90'],
            #    ['0','0.5','1.0','1.5','2.0','2.5']]#,'3.0','3.5','4.0','4.5','5.0','5.5','6.0','6.5','7.0']]
            #    ['0','20','40','60','80','100','120']]#,'3.0','3.5','4.0','4.5','5.0','5.5','6.0','6.5','7.0']]
    y_ticks = np.linspace(0,xy_limit[axtype[1]],9)

    # plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=40) #刻度
    # plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=40)

    # ax = plt.gca()
    xylabels = ['Number\ of\ Requests','Load\ Factor','Evict\ Ratio']
    xylabel_units = ['M','\%','\%']
    
    # ax.set_xlabel(' $\mathrm{' + xylabels[axtype[0]] + '\ (' + xylabel_units[axtype[0]] + ')}$') 
    # ax.set_ylabel(' $\mathrm{' + xylabels[axtype[1]] + '\ (' + xylabel_units[axtype[1]] + ')}$')
    # # ax.invert_yaxis()

    thelabels = [h.get_label() for h in lines]  
    plt.legend(lines, thelabels, prop=font_times, bbox_to_anchor=(0.45,-0.12),loc = 9, ncol = 3, frameon=False)

    # ax.legend() # 图例
    # bx.legend(prop=font_times, bbox_to_anchor=(0.45,-0.12),loc = 3, ncol = 4, frameon=False) # 图例
    plt.subplots_adjust(left=0.12, right=0.86, top=0.98, bottom=0.25)

    plt.savefig(csvdir + str(idx) + ".svg", format = "svg")
    plt.savefig(csvdir + str(idx) + "_image.jpg")
    plt.show()


files = []
logfiles = []
csvfiles = []
validfiles = []
labels = []
idx = sys.argv[2]
for argva in sys.argv[3:6]:
    # logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")
    print(argva)
    argva = re.sub('va',"_valid",argva)
    argva = re.sub('nv',"_invalid",argva)
    argva = re.sub('me',"_total",argva)

    # print(argva)
    res = re.findall('[a-zA-Z]+[0-9]*-*t*',argva)
    sep = " + "
    resstr = sep.join(res)
    print(resstr)
    dynamic_name = res[-1]
    # labels.append(res[0]) 
    labels.append(dynamic_name)
    # labels.append(argva)
    # if dynamic_name == "total":
        # labels.append("4/7 log") # valid ratio
    # if dynamic_name == "invalid":
        # labels.append("4/6 log") # valid ratio
    # if dynamic_name == "valid":
        # labels.append("8/7 log") # valid ratio


for argva in sys.argv[6:]:
    # logfiles.append(logdir + argva + ".log")
    validfiles.append(csvdir + argva + ".csv")
    print(argva)
    # labels.append(argva)

labels.append("4/7 log") # valid ratio
labels.append("4/6 log")
labels.append("8/7 log")

# validcsv = "/home/bwb/ACEKV/u3in1/valid.csv"
# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:
draw_insert_new(int(idx))

# draw_insert()
# draw()
# print(X,Y)

