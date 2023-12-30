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
# logdir = "/home/bwb/GPCode/proto-mica/data/"
csvdir = "/home/bwb/GPCode/proto-mica/data/tools/worktable_cdf/"
# csvdir = logdir + "mica/50s50z/fixedR/zipf80/"
# csvdir = sys.argv[1] + "/"
# nameIdx = [1,3,0,2,4]


config = {
    "font.family":'serif',
    "font.size": 50,
    "mathtext.fontset":'stix',
    "font.serif": ['SimSun'],
}
rcParams.update(config)

def draw_insert_new():

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
    Xs = []
    Ys = []
    
    for csvfile in csvfiles:
        with open(csvfile,"r") as f:
            lines = f.read().split("\n")
            X = []
            for line in lines:
                X.append(int(line))
            Xs.append(X)

    colors = ["red","blue","tab:purple"]

   
    linestyles = ['-','--','-']
    plt.figure(figsize=(15,10))
    markers = [' ',' ',' ']

    for i in range(0,len(Xs)):
        sorted_data = np.sort(Xs[i])
        print(sorted_data)
        yvals = np.arange(len(sorted_data))/float(len(sorted_data)-1)
        plt.plot(sorted_data, yvals,linewidth =3.0,label=labels[i],color=colors[i],linestyle=linestyles[i])
        # plt.plot(Xs[i],Xs[i],label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心

    print(plt.xlim(0,2000))
    print(plt.ylim(-0.05,1.05))


    x_ticks = np.linspace(0,2000,6)
    y_ticks = np.linspace(0,1.0,11)

    x_name = ['0','20','40','60','80','100']
    y_name = ['0.0','0.1','0.2','0.3','0.4','0.5','0.6','0.7','0.8','0.9','1.0']


    plt.xticks(x_ticks,x_name,fontsize=40) #刻度
    plt.yticks(y_ticks,y_name,fontsize=40)

    ax = plt.gca()
    
    ax.set_xlabel('写入请求延迟' + ' $\mathrm{( \mu s )}$') 
    ax.set_ylabel("$\mathrm{CDF}$")
    # ax.invert_yaxis()
    ax.legend(prop=font_times) # 图例
    ax.legend(prop=font_times, bbox_to_anchor=(0.49,-0.12),loc = 9, ncol = 6, frameon=False) # 图例
    # plt.subplots_adjust(left=0.1, right=0.9, top=0.9, bottom=0.3)
    plt.subplots_adjust(left=0.105, right=0.97, bottom=0.195, top=0.98)

    plt.savefig("cdf.svg", format = "svg")
    plt.savefig("cdf_image.jpg")
    plt.show()

csvfiles = []
labels = []

for argva in sys.argv[2:]:
    csvfiles.append(csvdir + argva + ".csv")
    labels.append(argva)
    

# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:
draw_insert_new()

# draw_insert()
# draw()
# print(X,Y)

