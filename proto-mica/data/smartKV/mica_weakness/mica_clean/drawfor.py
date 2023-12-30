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
    colors = ["#f05326","tab:purple","tab:red","tab:green","tab:gray","tab:brown"]
    # colors = ["tab:red","tab:orange","tab:purple","tab:blue","tab:green","tab:gray"]

   
    linestyles = ['-','-.','-.',':',':',':','-','-','-']
    plt.figure(figsize=(15,12))

    markers = ['o','^',' ',' ',' ',' ',' ',' ']
    for i in range(0,len(Xs)):
            plt.plot(Xs[i],Ys[i],linewidth =4.0,label=labels[i],linestyle=linestyles[i],marker=markers[i],markersize=17,color=colors[i],markerfacecolor='white',markeredgewidth=5) # markerfacecolor='white'可画空心
    temp_limit = 100
    if MY_define == 70 and (idx == 0 or idx == 3):
        temp_limit = 70
    xy_limit = [150000000, 100, temp_limit, 6000000]
    print(plt.xlim(0,xy_limit[axtype[0]]))
    if MY_define == 100 and (idx == 0 or idx == 3):
        print(plt.ylim(50,xy_limit[axtype[1]]))
    elif MY_define == 75 and (idx == 0 or idx == 3):
        print(plt.ylim(75,xy_limit[axtype[1]]))
    elif MY_define == 60 and (idx == 0 or idx == 3):
        print(plt.ylim(60,xy_limit[axtype[1]]))
    else:
        print(plt.ylim(0,xy_limit[axtype[1]]))  
    if xy_limit[axtype[0]] == 200:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)
    elif xy_limit[axtype[0]] == 400:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],5)
    elif xy_limit[axtype[0]] == 160000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],9)
    elif xy_limit[axtype[0]] == 180000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],10)
    elif xy_limit[axtype[0]] == 200000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],5)
    elif xy_limit[axtype[0]] == 250000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],6)
    elif xy_limit[axtype[0]] == 150000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],16)
    elif xy_limit[axtype[0]] == 300000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],7)
    elif xy_limit[axtype[0]] == 500000000:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)
    elif xy_limit[axtype[0]] == 100:
        x_ticks = np.linspace(0,xy_limit[axtype[0]],11)

    temp_list = ['0','10','20','30','40','50','60','70','80','90','100']
    if idx == 0 or idx == 3:
        if MY_define == 100:
            temp_list = ['50','55','60','65','70','75','80','85','90','95','100']
        elif MY_define == 75:
            temp_list = ['75','80','85','90','95','100']
        elif MY_define == 60:
            temp_list = ['60','65','70','75','80','85','90','95','100']
        elif MY_define == 70:
            # temp_list = ['0','5','10','15','20','25','30','35','40','45','50']
            temp_list = ['0','10','20','30','40','50','60','70']

    xy_name = [#['0','50','100','150','200','250','300'],#'350','400'],
            #    ['0','20','40','60','80','100','120','140','160','180','200'],
               ['0','10','20','30','40','50','60','70','80','90','100','110','120','130','140','150'],
               ['0','10','20','30','40','50','60','70','80','90','100'],
            #    ['0','10','20','30','40','50','60','70','80','90'],
                temp_list,
            #    ['0','10','20','30','40','50'],
            #    ['0','10','15','20','25','30','35','40','45','50','55','60','65','70','75'],
            #    ['50','55','60','65','70','75','80','85','90','95','100'],
            #    ['0.5','1.0','1.5','2.0','2.5','3.0','3.5','4.0']]#,'4.5','5.0','5.5','6.0']]
               ['1.0','2.0','3.0','4.0','5.0','6.0']]
    # y_ticks = np.linspace(0,500,20)
    if xy_limit[axtype[1]] == 100:
        if MY_define == 100 and (idx == 0 or idx == 3):
            y_ticks = np.linspace(50,xy_limit[axtype[1]],11)
        elif MY_define == 75 and (idx == 0 or idx == 3):
            y_ticks = np.linspace(75,xy_limit[axtype[1]],6)
        elif MY_define == 60 and (idx == 0 or idx == 3):
            y_ticks = np.linspace(60,xy_limit[axtype[1]],9)
        else:
            y_ticks = np.linspace(0,xy_limit[axtype[1]],11)
    elif xy_limit[axtype[1]] == 110:
        if MY_define == 100 and (idx == 0 or idx == 3):
            y_ticks = np.linspace(50,xy_limit[axtype[1]],12)
        elif MY_define == 75 and (idx == 0 or idx == 3):
            y_ticks = np.linspace(75,xy_limit[axtype[1]],7)
        else:
            y_ticks = np.linspace(0,xy_limit[axtype[1]],12)
    elif xy_limit[axtype[1]] == 3000000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],7)
    elif xy_limit[axtype[1]] == 50:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],6)
    elif xy_limit[axtype[1]] == 70:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],8)
    elif xy_limit[axtype[1]] == 75:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],15)
    elif xy_limit[axtype[1]] == 2500000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],6)
    elif xy_limit[axtype[1]] == 4000000:
        y_ticks = np.linspace(500000,xy_limit[axtype[1]],8)
    elif xy_limit[axtype[1]] == 4500000:
        y_ticks = np.linspace(0,xy_limit[axtype[1]],10)
    elif xy_limit[axtype[1]] == 6000000:
        y_ticks = np.linspace(1000000,xy_limit[axtype[1]],6)

    plt.xticks(x_ticks,xy_name[axtype[0]],fontsize=60) #刻度
    plt.yticks(y_ticks,xy_name[axtype[1]],fontsize=60)

    ax = plt.gca()
    xylabels = ['Number\ of\ Requests','Load\ Factor','Hit\ Rate','Throughput']
    xylabel_units = ['M','\%','\%','Mops']
    
    ax.set_xlabel(' $\mathrm{' + xylabels[axtype[0]] + '\ (' + xylabel_units[axtype[0]] + ')}$') 
    ax.set_ylabel(' $\mathrm{' + xylabels[axtype[1]] + '\ (' + xylabel_units[axtype[1]] + ')}$')
    # ax.invert_yaxis()
    ax.legend(prop=font_times, bbox_to_anchor=(0.45,-0.12),loc = 9, ncol = 2, frameon=False) # 图例
    # ax.legend(prop=font_times, bbox_to_anchor=(0.45,-0.15),loc = 9, ncol = 2, frameon=False) # 图例
    plt.subplots_adjust(left=0.16, right=0.953, top=0.975, bottom=0.215)
    # plt.subplots_adjust(left=0.16, right=0.953, top=0.97, bottom=0.3)

    plt.savefig(csvdir + str(nameIdx[idx]) + ".svg", format = "svg")
    plt.savefig(csvdir + str(nameIdx[idx]) + "_image.jpg")
    plt.show()

# MY_define = 100 #50-100
# MY_define = 75 #75-100
# MY_define = 60 #75-100
MY_define = 70 #0-70
# MY_define = 0 #0-100
files = []
logfiles = []
csvfiles = []
labels = []
# drawFuncIdx = [0,1,2,3,4]
# drawFuncIdx = [0,1,2,3,4]
idx = sys.argv[2]
for argva in sys.argv[3:]:
    # logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")
    print(argva)
    argva = re.sub('mz',"MICA-t_skewed",argva)
    argva = re.sub('mu',"MICA-t_uniform",argva)
    argva = re.sub('pz',"smartKV_skewed",argva)
    argva = re.sub('pu',"smartKV_uniform",argva)
    argva = re.sub('cz',"MemC3-t_skewed",argva)
    argva = re.sub('cu',"MemC3-t_uniform",argva)
    # print(argva)
    res = re.findall('[a-zA-Z]+[0-9]*-*t*',argva)
    sep = " + "
    resstr = sep.join(res)
    print(resstr)
    dynamic_name = res[0]
    # if res[2] == "clean":
    #     dynamic_name = dynamic_name + " w/ cleanup"
    # elif res[2] == "noclean":
    #     dynamic_name = dynamic_name + " w/o cleanup"
    # labels.append(res[0]) 
    labels.append(dynamic_name)
    # labels.append(argva)

# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# for idx in drawFuncIdx:
draw_insert_new(int(idx))

# draw_insert()
# draw()
# print(X,Y)

