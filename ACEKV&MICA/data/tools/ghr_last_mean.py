import os
import re
import sys
# from tkinter import BOTTOM
# import matplotlib.pyplot as plt
import numpy as np

filedir = sys.argv[1] + "/"
name = str(sys.argv[-1])

csvfile = filedir + name + ".csv"
def saveToCSV(X, Y):
    with open(csvfile,"w") as f:
        for i in range(0,len(X)):
            x = float(X[i])
            y = float(Y[i])
            line = "{:.1f}\t{:.4f}\n".format(x,y)
            f.write(line) 
    # 去除末尾空行
    with open(csvfile, 'r', newline='') as file:
        data = file.readlines()
        data = [line.rstrip() for line in data]
    # 将转换后的数据写入新的CSV文件
    with open(csvfile, 'w', newline='') as file:
        file.write('\n'.join(data))

def data_mean(idx,str0,str2):
    Xs = []
    Ys = []
    ix = 10
    ii = 1
    for file in files:
        Y = []
        cnt = 0
        with open(file,"r") as f:
            lines = f.read().split("\n")
            # for line in lines:
            for i in range(len(lines)-1, -1, -1):
                line = lines[i]
                words = re.split(r"[ ]+", line)
                # if words[0] == "[P]total" and words[2] == "path:":
                if words[0] == str0 and words[2] == str2:
                    Y.append(float(words[idx]))
                    cnt += 1
                    if cnt == 10:
                        break
        # print("{:.7f}".format(Y[0]))
        print(ix, end="\t")
        Xs.append(ix)
        print("{:.4f}".format(np.mean(Y)))
        Ys.append("{:.4f}".format(np.mean(Y)))
        if ii % 3 == 0:
            ix += 10
        ii += 1
    return Xs, Ys

files = []

for argva in sys.argv[2:-1]:
    files.append(filedir + argva + ".log")
X = []
Y = []
X, Y = data_mean(3,"[P]Get","Rate:")
saveToCSV(X, Y)