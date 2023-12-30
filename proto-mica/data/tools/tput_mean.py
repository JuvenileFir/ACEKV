import os
import re
import sys
# from tkinter import BOTTOM
# import matplotlib.pyplot as plt
import numpy as np
# logdir = "/home/bwb/GPCode/proto-mica/data/refine_files/"
# csvdir = "/home/bwb/GPCode/proto-mica/data/refine_files/res_files/"


csvdir = sys.argv[1] + "/"


def data_mean():


    Xs = []
    Ys = []
    
    for csvfile in csvfiles:
        # print(csvfile)
        Y = []
        with open(csvfile,"r") as f:
            lines = f.read().split("\n")
            # Y = []
            for line in lines:
                words = re.split(r"\t+", line)
                if float(words[1]) <= 50 and float(words[3]) >= 3000000:
                # if float(words[1]) <= 50:
                # if float(words[3]) >= 3000000:
                    Ys.append(float(words[3]))
            # Ys.append(Y)
    # print(np.mean(Ys[0]))
    print(np.mean(Ys))

files = []
csvfiles = []

for argva in sys.argv[2:]:
    csvfiles.append(csvdir + argva + ".csv")


data_mean()