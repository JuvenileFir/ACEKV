import os
import re
import sys
import numpy as np


csvdir = sys.argv[1] + "/"


def data_mean():
    Xs = []
    Ys = []
    for csvfile in csvfiles:
        Y = []
        with open(csvfile,"r") as f:
            lines = f.read().split("\n")
            for line in lines:
                words = re.split(r"\t+", line)
                Ys.append(float(words[0]))
        print(np.mean(Ys))

files = []
csvfiles = []

for argva in sys.argv[2:]:
    csvfiles.append(csvdir + argva + ".csv")

data_mean()