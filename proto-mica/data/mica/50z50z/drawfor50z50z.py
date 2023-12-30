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

def getInsertData(logfile):
    X = []
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "APPEND":
                if not words[1].isnumeric():
                    print("[+]ERROR: get insert num error")
                    print(line)
                    continue
                insert_num = int(words[1])
                used_time = float(words[3])
                Y.append(used_time)
                X.append(insert_num)
    return X,Y


def draw_tpc(X,Y1):
    # plt.plot(X,Y1,label="Bindex-tpc-date-scan",linestyle='-',marker='o',color='red')
    # plt.plot(X,Y2,label="Cs-tpc-date-scan",linestyle='-',marker='v',color='blue')
    rawX = X
    X = np.arange(len(X)) + 1
    total_width, n = 0.8 , 2
    width = total_width / n
    X = X - (total_width - width) / 2
    
    plt.bar(X,Y1,width=width,label="Bindex-tpc-date-scan",color='orange')
    # plt.bar(X+width,Y2,width=width,label="Cs-tpc-date-scan",color='cornflowerblue')
    # print(Y1)
    print(plt.xlim(0,10))
    print(plt.ylim(0,0.1))
    x_ticks = np.linspace(0,24,25)
    y_ticks = np.linspace(0,0.5,20)
    plt.xticks(x_ticks)
    plt.yticks(y_ticks)

    ax = plt.gca()

    ax.set_xlabel('query idx')
    ax.set_ylabel('use time (ms)')
    # ax.set_title("Bindex1 & Bindex2: Scan width {}".format(width))
    ax.legend()

    plt.show()


def draw_insert():
    BAR = False
    files = []
    for argva in sys.argv[1:]:
        files.append(argva)
    Ys = []
    for file in files:
        X,Y1 = getAppendTimeCount("/home/lym/Code/bindex-plus/log/{}".format(file))
        print("####################")
        print(Y1)
        Y1 = refine_data(Y1)
        print(Y1)
        Y1 = mergeData(10,Y1)
        print("####################")
        Ys.append(Y1)
    
    X = [1,2,3,4,5,6,7,8,9,10]
    colors = ["pink","red","blue","green","yellow","cyan","lightgreen","chocolate"]
    labels = [128,256,512,1024,2048,4096,8192,16384]
    # labels = ["before","after"]
    # plt.plot(X,Y1,label="Bindex-unsorted-slotarray",linestyle='-',marker='o',color='cyan')
    
    if not BAR:
        for i in range(0,len(Ys)):
            plt.plot(X,Ys[i],label=labels[i],linestyle='-',marker='o',color=colors[i])
    else:
        X = np.arange(10) + 1
        total_width, n = 0.8 , len(Ys)
        width = total_width / n
        X = X - (total_width - width) / 2
        for i in range(0,len(Ys)):
            plt.bar(X + width * i,Ys[i],width=width,label=labels[i],color=colors[i])

    # print("unsorted-bitmapVSnobitmap")
    # cal_avg_and_up(X, Y1, Y2, "sorted-scan", "unsorted-scan")


    print(plt.xlim(0,10))
    print(plt.ylim(0,0.1))
    x_ticks = np.linspace(1,10,10)
    x_name = [10,20,30,40,50,60,70,80,90,100]
    # y_ticks = np.linspace(0,500,20)
    y_ticks = np.linspace(0,550,20)
    plt.xticks(x_ticks,x_name)
    plt.yticks(y_ticks)

    ax = plt.gca()

    ax.set_xlabel('insert data %')
    ax.set_ylabel('use time (ms)')
    # ax.set_title("Bindex1 & Bindex2: Scan width {}".format(width))
    ax.legend()

    plt.show()
    plt.savefig("insert_blocksize.png")


def transformFromMsToS(Y):
    newY = []
    for y in Y:
        newY.append(y/1000)
    return newY


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

            # del Y[:]
        # print("####################")
        # print(Y1)
        # Y1 = refine_data(Y1)
        # print(Y1)
        # Y1 = mergeData(5,Y1)
        # print("####################")
        # for y in Y1:
        #     print(y)
        # Ys.append(Y1)


       
    # newYs = []
    # for Y in Ys:
        # Y = transformFromMsToS(Y)
        # newYs.append(Y)
    # Ys = newYs
    # print(Ys)
    colors = ["red","blue"]
   
    linestyles = ['-','-']
    plt.figure(figsize=(12.8,7.2))
    markers = [' ',' ']
    for i in range(0,len(Xs)):
        plt.plot(Xs[i],Ys[i],label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=8,color=colors[i]) # markerfacecolor='white'可画空心

    xy_limit = [400, 80, 110, 6000000]
    print(plt.xlim(0,xy_limit[axtype[0]]))
    print(plt.ylim(0,xy_limit[axtype[1]]))
    if xy_limit[axtype[0]] == 80:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],9)
    elif xy_limit[axtype[0]] == 400:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],9)

    xy_name = [['0','50','100','150','200','250','300','350','400'],
               ['0','10','20','30','40','50','60','70','80'],
               ['0','10','20','30','40','50','60','70','80','90','100','110'],
               ['0','0.5','1.0','1.5','2.0','2.5','3.0','3.5','4.0','4.5','5.0','5.5','6.0']]
    # y_ticks = np.linspace(0,500,20)
    if xy_limit[axtype[1]] == 110:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],12)
    elif xy_limit[axtype[1]] == 80:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],9)
    elif xy_limit[axtype[1]] == 6000000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],13)

    plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=19) #刻度
    plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=19)

    ax = plt.gca()
    xylabels = ['时间','索引装载率','读取命中率','吞吐量']
    xylabel_units = ['s','\%','\%','Mops']
    
    # ax.set_xlabel('Insert Data %',font)
    # ax.set_title("protoKV vs mica*的命中率变化趋势" ,font)
    ax.set_title('$\mathrm{proto\ &\ mica*}$的' + xylabels[axtype[1]] + '随' + xylabels[axtype[0]] + '的变化趋势')
    ax.set_xlabel(xylabels[axtype[0]] + ' $\mathrm{(' + xylabel_units[axtype[0]] + ')}$') 
    ax.set_ylabel(xylabels[axtype[1]] + ' $\mathrm{(' + xylabel_units[axtype[1]] + ')}$')
    # ax.invert_yaxis()
    ax.legend(prop=font_times) # 图例

    plt.savefig(csvdir + str(idx) + ".svg", format = "svg")
    plt.savefig(csvdir + str(idx) + "_image.jpg")
    plt.show()


files = []
logfiles = []
csvfiles = []
labels = []
drawFuncIdx = [0,1,2,3,4]
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

