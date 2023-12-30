import matplotlib.pyplot as plt
import numpy as np

# x = np.linspace(0, 10, 100)
x = [[1,2,3,4,5,6,7,8,9],[1.01,2.02,3.01,4.01,5.01,6.01,7.01,8.01,9.01]]
lines = [[1,2,3,4,5,4,3,2,1],[5,4,3,2,1,4,6,8,9]]


# 绘制每条线，并确保不首尾相连
for i in range(0,len(lines)):
    plt.plot(x[i], lines[i], linestyle='-', marker='o')

# 使用plt.show()只显示最后一个图形
plt.show()
plt.savefig("test_image.png")

