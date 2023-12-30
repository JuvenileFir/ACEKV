import os
import re
import sys
# from tkinter import BOTTOM
# import matplotlib.pyplot as plt
import numpy as np

filedir = sys.argv[1] + "/"
def tput_mean():
    Ys = []
    for file in files:
        Y = []
        thres = 1000.0
        with open(file,"r") as f:
            lines = f.read().split("\n")
            # for line in lines:
            for i in range(len(lines)-1, -1, -1):
                line = lines[i]
                words = re.split(r"[ ]+", line)
                if words[0] == "[P]indexs":
                    thres = float(words[3])
                if thres < 99.9 and words[0] == "[P]Throughput:":
                    Ys.append(float(words[1]))
    print("[P]tput:", end=" ")
    print("{:.2f}".format(np.mean(Ys)))


def data_mean(idx,str0,str2):
    Ys = []
    for file in files:
        Y = []
        thres = 1000.0
        with open(file,"r") as f:
            lines = f.read().split("\n")
            # for line in lines:
            for i in range(len(lines)-1, -1, -1):
                line = lines[i]
                words = re.split(r"[ ]+", line)
                if words[0] == "[P]indexs":
                    thres = float(words[3])
                # if words[0] == "[P]total" and words[2] == "path:":
                if thres < 99.9 and words[0] == str0 and words[2] == str2:
                    Ys.append(float(words[idx]))
                    break
    Q1 = np.percentile(Ys, 25)
    Q3 = np.percentile(Ys, 75)
    
    # 计算上下限
    IQR = Q3 - Q1
    upper_limit = Q3 + 1.5 * IQR
    lower_limit = Q1 - 1.5 * IQR
    
    # 去除异常值
    # cleaned_data = [x for x in Ys if (x > lower_limit) and (x < upper_limit)]
    
    #不去除
    cleaned_data = Ys

    print(str0 + " " + str2, end=" ")
    if re.findall("per",str2):
        print("{:.7f}".format(np.mean(cleaned_data)))
    else:
        print(int(np.mean(cleaned_data)))

files = []

for argva in sys.argv[2:]:
    files.append(filedir + argva + ".log")

data_mean(3,"[P]total","path:")
data_mean(3,"[P]total","num:")
data_mean(3,"[P]total","percuckoo:")
data_mean(3,"[P]total","perset:")
# tput_mean()
print(" ")