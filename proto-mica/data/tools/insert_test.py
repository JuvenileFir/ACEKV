import os
import re
import sys
# from tkinter import BOTTOM
# import matplotlib.pyplot as plt
# import numpy as np
logdir = "/home/bwb/GPCode/proto-mica/data/"
csvdir = logdir + "mica/"
def singleTest(logfile):

    # clean output file
    cmd = "echo > {}".format(logfile)

    os.system(cmd)

    for insert_num in range(10,100,10):
        print("insert num: ",insert_num)
        cmd = "sudo ./bindex -i -n {} >> {}".format(insert_num,logfile)
        os.system(cmd)

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

""" def saveToCSV(csvfile, X, Y):
    with open(csvfile,"w") as f:
        for i in range(0,len(X)):
            x = X[i]
            y = Y[i]
            line = "{} {}\n".format(x,y)
            f.write(line) """

def savesingleToCSV(csvfile, X):
    with open(csvfile,"w",newline="") as f:
        for i in range(0,len(X)):
            x = X[i]
            line = "{}\n".format(x)
            f.write(line)

def savedoubleToCSV(csvfile, X, Y):
    with open(csvfile,"w",newline="") as f:
        for i in range(0,len(X)):
            x = X[i]
            y = Y[i]
            z = Z[i]
            line = "{}\t{}\n".format(x,y)
            f.write(line) 

def savetripleToCSV(csvfile, X, Y, Z):
    with open(csvfile,"w",newline="") as f:
        for i in range(0,len(X)):
            x = X[i]
            y = Y[i]
            z = Z[i]
            line = "{}\t{}\t{}\n".format(x,y,z)
            f.write(line)            

def savequadrToCSVwithTime(csvfile, T, X, Y, Z):
    with open(csvfile,"w") as f:
        for i in range(0,len(X)):
            t = T[i] - T[0] + 1.0
            x = X[i]
            y = Y[i]
            z = Z[i]
            line = "{:.2f}\t{:.4f}\t{:.4f}\t{:.1f}\n".format(t,x,y,z)
            f.write(line) 
    # 去除末尾空行
    with open(csvfile, 'r', newline='') as file:
        data = file.readlines()
        data = [line.rstrip() for line in data]
    # 将转换后的数据写入新的CSV文件
    with open(csvfile, 'w', newline='') as file:
        file.write('\n'.join(data))

def savequadrToCSV(csvfile, T, X, Y, Z):
    with open(csvfile,"w") as f:
        for i in range(0,len(X)):
            t = T[i]
            x = X[i]
            y = Y[i]
            z = Z[i]
            line = "{:<10d}\t{:.4f}\t{:.4f}\t{:.1f}\n".format(t,x,y,z)
            f.write(line) 
    # 去除末尾空行
    with open(csvfile, 'r', newline='') as file:
        data = file.readlines()
        data = [line.rstrip() for line in data]
    # 将转换后的数据写入新的CSV文件
    with open(csvfile, 'w', newline='') as file:
        file.write('\n'.join(data))

def savepentaToCSV(csvfile, T, X, Y, Z, P):
    with open(csvfile,"w") as f:
        for i in range(0,len(X)):
            t = T[i]
            x = X[i]
            y = Y[i]
            z = Z[i]
            p = P[i]
            line = "{:<10d}\t{:.4f}\t{:.4f}\t{:.1f}\t{:.1f}\n".format(t,x,y,z,p)
            f.write(line) 
    # 去除末尾空行
    with open(csvfile, 'r', newline='') as file:
        data = file.readlines()
        data = [line.rstrip() for line in data]
    # 将转换后的数据写入新的CSV文件
    with open(csvfile, 'w', newline='') as file:
        file.write('\n'.join(data))


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

def mergeData(pointNum,Y):
    result = []
    blockSize = int(len(Y) / pointNum)
    print(blockSize)
    for i in range(0,pointNum):
        blockSum = 0
        for j in range(0,blockSize):
            blockSum += float(Y[i * blockSize + j])
        result.append(blockSum / blockSize)
    return result

def refine_data(data):
    var = np.var(data)
    avg = np.mean(data)
    strip_data = []
    for i in range(0,len(data)):
        # if (i - avg)**2 > var * 10:
        if (data[i] - avg) / avg > 1.2:
            strip_data.append(i)
    for i in strip_data:
        data[i] = avg
    return data

def getAppendTimeCount(logfile,csvfile):
# def getAppendTimeCount(logfile):
    X = []
    Y = []
    count = 10
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[Time]":
                if words[1] == "append" and words[3] == "bindex:":
                    used_time = float(words[-1])
                    Y.append(used_time)
                    X.append(count)
                    count += 10
    _csvfile = "./append_to_bindex_time/atb_" + csvfile
    savesingleToCSV(_csvfile, Y)
    # return X,Y

# def getcontent(csvfile):
#     with open(csvfile,"r") as f:
#         return (f.read()).split("\n")

def getInsertTimeCount(logfile,csvfile):
    X = []
    Y = []
    count = 10
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[Time]":
                if words[1] == "insert" and words[3] == "block:":
                    used_time = float(words[-1])
                    Y.append(used_time)
                    X.append(count)
                    count += 10
    _csvfile = "./insert_to_block_time/itb" + csvfile
    savesingleToCSV(_csvfile, Y)
    # saveToCSV(_csvfile,X,Y)    
    # return X,Y

def getSplitTimeCount(logfile,csvfile):
    X = []
    Y = []
    count = 10
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[Time]":
                if words[1] == "split" and words[2] == "block:":
                    used_time = float(words[-1])
                    Y.append(used_time)
                    X.append(count)
                    count += 10
    _csvfile = "./split_block_time/sb" + csvfile
    savesingleToCSV(_csvfile, Y)
    # saveToCSV(_csvfile,X,Y)    
    # return X,Y


def getSplitCount(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "SPLIT" and words[1] == "COUNT:":
                used_time = float(words[-1])
                Y.append(used_time)
    _csvfile = "./split_block_time/split_count_" + csvfile
    savesingleToCSV(_csvfile, Y)
    # saveToCSV(_csvfile,X,Y)    
    # return X,Y
#
#获取log内容模板函数
#
def getContent(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]index":
                used_time = float(words[3])
                Y.append(used_time)
    _csvfile = "./memc3/ilf_" + csvfile
    savesingleToCSV(_csvfile, Y)

def getiLF(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]indexs": # [P]index / indexs
                used_time = float(words[3])
                Y.append(used_time)
    # _csvfile = "./ilf/ilf_" + csvfile #./memc3/ilf_
    # savesingleToCSV(_csvfile, Y)
    return Y
def getsLF(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]slabs": # [P]slabs / cirlog
                used_time = float(words[3])
                Y.append(used_time)
    _csvfile = "./memc3/slf_" + csvfile
    savesingleToCSV(_csvfile, Y)
def getghr(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]Get":
                used_time = float(words[3])
                if used_time > 100.0:
                    used_time = 100.0
                Y.append(used_time)
    # _csvfile = "./ghr/ghr_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def getmicaevict(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]mica" and words[1] == "evicted:":
                used_time = float(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def getvalid(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]Valid" and words[1] == "Entries:":
                used_time = float(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def getsetsucc(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]set" and words[1] == "success:":
                used_time = float(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def getsetput(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]SetThroughput:":
                used_time = float(words[1])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def gettput(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]Throughput:":
                used_time = float(words[1])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y

def getsetreq(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]set" and words[1] == "request:":
                used_time = int(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
    
def getallreq(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[P]all" and words[1] == "request:":
                used_time = int(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y

def getctime(logfile,csvfile):
    Y = []
    with open(logfile,"r") as f:
        data = f.read()
        lines = data.split("\n")
        for line in lines:
            words = re.split(r"[ ]+", line)
            if words[0] == "[INFO]Current":
                used_time = float(words[2])
                Y.append(used_time)
    # _csvfile = "./tput/tput_" + csvfile
    # savesingleToCSV(_csvfile, Y)
    return Y
def transformFromMsToS(Y):
    newY = []
    for y in Y:
        newY.append(y/1000)
    return newY

def draw_insert_new():

    font = {
        # "family":"cursive",
        "weight":"normal",   # "bold"
        "size":19,
    }
    
    BAR = True
    # files = []
    # for argva in sys.argv[1:]:
    #     files.append(argva)
    Ys = []
    # for file in files:
        # X,Y1 = getAppendTimeCount(logfile,csvfile)
        # X,Y1 = getAppendTimeCount("/home/lym/Code/bindex-plus/log/{}".format(file))
    for csvfile in csvfiles:
        _csvfile = "./append_to_bindex_time/" + csvfile
        Y1 = getcontent(_csvfile)
        print("####################")
        print(Y1)
        # Y1 = refine_data(Y1)
        # print(Y1)
        Y1 = mergeData(5,Y1)
        print("####################")
        for y in Y1:
            print(y)
        Ys.append(Y1)
    

    newYs = []
    for Y in Ys:
        Y = transformFromMsToS(Y)
        newYs.append(Y)
    Ys = newYs
    print(Ys)
    X = [1,2,3,4,5]
    colors = ["pink","red","blue","green","yellow","cyan","lightgreen","chocolate"]
    # colors = ["pink","green","yellow","cyan","lightgreen","chocolate"]
    # labels = [128,256,512,1024,2048,4096,8192,16384]
    labels = ["Baseline","Sorted-Batch","Unsorted-Batch","Unsorted-indiv"]
    # labels = ["Sorted","Unsorted"]
    # plt.plot(X,Y1,label="Bindex-unsorted-slotarray",linestyle='-',marker='o',color='cyan')
    
    # plt.figure(figsize=(12.8,7.2))
    plt.figure(figsize=(8,6))
    if not BAR:
        markers = ['o', 'x', '^', 's', '+']
        for i in range(0,len(Ys)):
            plt.plot(X,Ys[i],label=labels[i],linestyle='-',marker=markers[i],markersize=8,color="black") # markerfacecolor='white'可画空心
    else:
        hatches = ['\\','.','/','o', 'x',  '*', 'O','-', '+', ]
        X = np.arange(5) + 1
        total_width, n = 0.8 , len(Ys)
        width = total_width / n
        X = X - (total_width - width) / 2
        for i in range(0,len(Ys)):
            plt.bar(X + width * i,Ys[i],width=width,label=labels[i],color="black",hatch=hatches[i],fill=False)

    # print("unsorted-bitmapVSnobitmap")
    # cal_avg_and_up(X, Y1, Y2, "sorted-scan", "unsorted-scan")


    print(plt.xlim(0,6))
    print(plt.ylim())
    x_ticks = np.linspace(1,5,5)
    x_name = ['1%','10%','20%','30%','40%']
    # y_ticks = np.linspace(0,500,20)
    y_ticks = np.linspace(0,40,5)
    plt.xticks(x_ticks,x_name,fontsize=19)
    plt.yticks(y_ticks,fontsize=19)

    ax = plt.gca()

    # ax.set_xlabel('Insert Data %',font)
    ax.set_xlabel('The amount of inserted data',font) 
    ax.set_ylabel('Time (s)',font)
    # ax.invert_yaxis()
    # ax.set_title("Bindex1 & Bindex2: Scan width {}".format(width))
    ax.legend(prop={'size': 19})

    plt.show()
    plt.savefig("insert_new.png")
    """ logfile = "log/bindex-DRAM-insert-10-100-1.log"
csvfile = "log/bindex-DRAM-insert-10-100-1.csv"
# singleTest(logfile)
X,Y = getInsertData(logfile)
saveToCSV(csvfile,X,Y) """
""" 
X,Y = getAppendTimeCount("/home/lym/Code/bindex-baseline-pmdk/log/1-26-bindex-baseline-append-2.log")
print(X,Y)
saveToCSV("log/1-26-bindex-baseline-append-2.csv",X,Y)

X,Y = getAppendTimeCount("/home/lym/Code/bindex-plus/log/1-26-bindex-plus-append-nobatch-sorted-2.log")
saveToCSV("log/1-26-bindex-plus-append-nobatch-sorted-2.csv",X,Y)

X,Y = getAppendTimeCount("/home/lym/Code/bindex-plus/log/1-26-bindex-plus-append-nobatch-unsorted-2.log")
saveToCSV("log/ 1-26-bindex-plus-append-nobatch-unsorted-2.csv",X,Y)
 """

files = []
logfiles = []
csvfiles = []
for argva in sys.argv[1:]:
    logfiles.append(logdir + argva + ".log")
    csvfiles.append(csvdir + argva + ".csv")

# logfile = sys.argv[1] + ".log"
# csvfile = sys.argv[1] + ".csv"

# X,Y = getAppendTimeCount(logfile)
# saveToCSV(csvfile,X,Y)

for i in range(0,len(logfiles)):
    # 下面四行提取indexLF、get hit rate和tput并写入文件
    # T = getctime(logfiles[i],csvfiles[i])
    # T = getsetreq(logfiles[i],csvfiles[i])
    T = getallreq(logfiles[i],csvfiles[i])
    X = getiLF(logfiles[i],csvfiles[i])
    Y = getghr(logfiles[i],csvfiles[i])
    P = gettput(logfiles[i],csvfiles[i])
    # Z = getsetput(logfiles[i],csvfiles[i])
    # P = getsetsucc(logfiles[i],csvfiles[i])
    # P = getvalid(logfiles[i],csvfiles[i])
    # P = getmicaevict(logfiles[i],csvfiles[i])
    # savepentaToCSV(csvfiles[i], T, X, Y, Z, P)
    savequadrToCSV(csvfiles[i], T, X, Y, P)
    # savequadrToCSVwithTime(csvfiles[i], T, X, Y, Z)
    # savetripleToCSV(csvfiles[i], X, Y, Z)
    # 下面两行单独提取时间并写入文件
    # X = getctime(logfiles[i],csvfiles[i])
    # savesingleToCSV(csvfiles[i], X)

# for i in range(0,len(logfiles)):
    # getSplitCount(logfiles[i],csvfiles[i])

# draw_insert_new()

# draw_insert()
# draw()
# print(X,Y)

