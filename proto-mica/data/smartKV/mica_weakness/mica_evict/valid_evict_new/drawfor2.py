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
       
    # newYs = []
    # for Y in Ys:
        # Y = transformFromMsToS(Y)
        # newYs.append(Y)
    # Ys = newYs
    # print(Ys)
    # colors = ["red","tab:purple","tab:orange","tab:blue","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]
    colors = ["#f05326","tab:purple","tab:red","tab:green","tab:gray","tab:brown"]
   
    linestyles = ['-','-.','-.',':',':',':','-','-','-']
    # fig, ax = plt.subplots(figsize=(15,12))
    plt.figure(figsize=(15,12))

    markers = ['o','^',' ',' ',' ',' ',' ',' ']
    # lines = bx.plot(Xsb[0],Ysb[0],linewidth =4.0,label=labels[3],linestyle=linestyles[3],marker=markers[3],markersize=8,color=colors[3]) # markerfacecolor='white'可画空心
    # lines = lines + ax.plot(Xs[0],Ys[0],linewidth =4.0,label=labels[0],linestyle=linestyles[0],marker=markers[0],markersize=8,color=colors[0]) # markerfacecolor='white'可画空心
    lines = []
    for i in range(0,len(Xs)):
        plt.plot(Xs[i],Ys[i],linewidth =4.0,label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=17,color=colors[i],markerfacecolor='white',markeredgewidth=5) # markerfacecolor='white'可画空心
        # plt.plot(Xs[i],Ys[i],linewidth =4.0,label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心

    xy_limit = [300000000, 100, 82]
    plt.xlim(-5,105)
    plt.ylim(-0.5,10.5)
    x_ticks = np.linspace(0,100,11)
    y_ticks = np.linspace(0,10,11)
    xy_name = [['0','10','20','30','40','50','60','70','80','90','100'],
               ['0','1','2','3','4','5','6','7','8','9','10']]
            #    ['0','10','20','30','40','50','60','70','80','90','100']]
    

    plt.xticks(x_ticks,xy_name[0],fontsize=60) #刻度
    plt.yticks(y_ticks,xy_name[1],fontsize=60)

    ax = plt.gca()
    xylabels = ['请求数','$\mathrm{\ Load\ Factor}$','$\mathrm{Average\ cuckoo\ path}$','吞吐量']
    xylabel_units = ['M','\%','\%','Mops']
    
    ax.set_xlabel('$\mathrm{ Load\ Factor(\%)}$') 
    ax.set_ylabel('$\mathrm{ Valid\ Index\ Eviction\ (\%)}$') 

    # ax.legend(prop=font_times, loc = 0, ncol = 2, frameon=True) # 图例 //bbox_to_anchor=(0.45,-0.12),
    ax.legend(prop=font_times, bbox_to_anchor=(0.45,-0.12),loc = 9, ncol = 2, frameon=False) # 图例
    plt.subplots_adjust(left=0.125, right=0.99, top=0.975, bottom=0.215)

    plt.savefig(csvdir + str(idx) + ".svg", format = "svg")
    plt.savefig(csvdir + str(idx) + "_image.jpg")
    plt.show()


files = []
logfiles = []
csvfiles = []
labels = []
idx = sys.argv[2]
for argva in sys.argv[3:]:
    # logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")
    print(argva)
    argva = re.sub('va',"_MICA-t",argva)
    argva = re.sub('nv',"_invalid",argva)
    argva = re.sub('me',"_total",argva)
    argva = re.sub('c3',"_MemC3-t",argva)

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


# validcsv = "/home/bwb/ACEKV/u3in1/valid.csv"
# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:
draw_insert_new(int(idx))

# draw_insert()
# draw()
# print(X,Y)

