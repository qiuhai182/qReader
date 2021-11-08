# -*- coding: UTF-8 -*-
# shell内核调用方式： python server.py "userId"

import os
from sklearn.cluster import KMeans, DBSCAN
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import sys
import math
import json
from dateutil.parser import parse
import seaborn as sns

sns.set()

# 计算两时间差的总秒数


# 计算两时间差秒数
def getDeltaTime(t1, t2):
    t1 = parse(t1)
    t2 = parse(t2)
    return (t2 - t1).total_seconds()

# 计算两点距离


def getDeltaDistance(x1, y1, x2, y2):
    return np.sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1))

# 计算两点连线相对x轴逆时针方向角度


def getAngle(x1, y1, x2, y2):
    angle = 0.0
    deltaX = x2 - x1
    deltaY = y2 - y1
    if x2 == x1:
        if y2 == y1:
            angle = 0.0
        elif y2 < y1:
            angle = 3.0 * math.pi / 2.0
        elif y2 > y1:
            angle = math.pi / 2.0
    elif x2 > x1 and y2 > y1:
        angle = math.atan(deltaX / deltaY)
    elif x2 > x1 and y2 < y1:
        angle = math.pi / 2 + math.atan(-deltaY / deltaX)
    elif x2 < x1 and y2 > y1:
        angle = 3.0 * math.pi / 2.0 + math.atan(deltaY / -deltaX)
    elif x2 < x1 and y2 < y1:
        angle = math.pi + math.atan(deltaX / deltaY)
    return 180 * angle / math.pi

# 计算两点角度变化值


def getDeltaAngle(x1, y1, x2, y2):
    return getAngle(0, 0, x2, y2) - getAngle(0, 0, x1, y1)


class read:
    def __init__(self, csv_data):
        self.allDataLength = len(csv_data)  # 总视线点数
        # 剔除屏幕外点
        csv_data = csv_data[0 < csv_data['x']]
        csv_data = csv_data[csv_data['x'] < 1024]
        csv_data = csv_data[0 < csv_data['y']]
        csv_data = csv_data[csv_data['y'] < 1366]
        csv_data.index = np.array(range(csv_data.shape[0]))
        self.outScreenDataLength = self.allDataLength - len(csv_data)  # 屏幕外点个数
        # 取x、y、time、pageNum
        self.X = np.array(csv_data['x'])
        self.Y = np.array(csv_data['y'])
        self.Time = np.array(csv_data['timeStamp'])
        self.PageNum = np.array(csv_data['pageNum'])
        self.Pages = 0  # 阅读页数
        self.Rows = 0  # 阅读行数
        self.droppedDataLength = 0  # 被剔除的连续重复点数
        self.latestCounting = True  # 是否正在计算最近一次阅读时长
        self.latestReadSeconds = 0.0  # 最近一次阅读时长总秒数
        # 倒序剔除连续重复点
        for i in range(len(csv_data) - 1, 0, -1):
            # 相较前一个点，x、y、页数 均未变化，则剔除该点
            if (self.X[i] == self.X[i - 1] and self.Y[i] == self.Y[i - 1] and self.PageNum[i] == self.PageNum[i - 1]):
                csv_data.drop(csv_data.index[[i]], axis=0)  # 剔除连续重复点
                self.droppedDataLength += 1  # 被剔除的连续重复点数+1
            else:
                # 临近两条数据相隔超过10分钟(600秒)，表名不是同一批次阅读数据，应结束计算最近一次阅读时长
                if (self.latestCounting and getDeltaTime(self.Time[i], self.Time[i - 1]) < 600):
                    self.latestReadSeconds += 0.1  # 最近一次阅读时长+0.1
                else:
                    self.latestCounting = False  # 停止计算最近一次阅读时长
                # 页数不一致 => 翻页
                if (self.PageNum[i] != self.PageNum[i - 1]):
                    self.Pages += 1  # 页数+1
                # 相较1秒前的点的距离，点左移且超过屏幕宽度一半 => 换行
                if (i > 10 and self.X[i - 10] - self.X[i] > 600):
                    self.Rows += 1  # 行数+1
        self.userfulDataLength = len(csv_data)  # 有效视线点数
        # 重定义X、Y、Time、Page
        self.X = np.array(csv_data['x'])
        self.Y = np.array(csv_data['y'])
        self.Time = np.array(csv_data['timeStamp'])
        self.PageNum = np.array(csv_data['pageNum'])
        self.BookId = np.array(csv_data['bookId'])
        self.time = len(self.X) / 10  # 收集每条有效数据厉时0.1秒，此为计算总秒数
        time = self.time
        self.sec = math.floor(math.floor(time) % 60)  # 阅读时长除去分钟数的余下的秒数(<60)
        time = math.floor((time - self.sec) / 60)  # 阅读整的总分钟数
        self.min = time % 60  # 阅读时长除去小时数的余下的分钟数(<60)
        self.hour = time = math.floor(time / 60)  # 阅读整的小时数
        self.focus = self.userfulDataLength / \
            (self.userfulDataLength +
             self.outScreenDataLength)  # 专注度 = 有效数据量 / 总数据量(有效+屏幕外)

        self.deltaX = np.array([], float)  # x变化量
        self.deltaY = np.array([], float)  # y变化量
        self.deltaDistance = np.array([], float)  # 距离变化量
        self.deltaAngle = np.array([], float)  # 角度变化量
        # 遍历取x2-x1,y2-y1,t2-t1,angle2-angle
        for i in range(csv_data.shape[0] - 1):
            x1 = csv_data['x'][i]
            y1 = csv_data['y'][i]
            x2 = csv_data['x'][i+1]
            y2 = csv_data['y'][i+1]
            self.deltaX = np.append(self.deltaX, np.array(x2 - x1, float))
            self.deltaY = np.append(self.deltaY, np.array(y2 - y1, float))
            self.deltaDistance = np.append(self.deltaDistance, np.array(
                getDeltaDistance(x1, y1, x2, y2), float))
            self.deltaAngle = np.append(self.deltaAngle, np.array(
                getDeltaAngle(x1, y1, x2, y2), float))

        # 阅读散点图
        xy = list(zip(self.deltaX, self.deltaY))
        XY = np.array(xy)
        cluster = KMeans(n_clusters=3).fit(XY)  # 构造聚类器
        label_pred = cluster.labels_  # 获取聚类标签
        self.point0 = XY[label_pred == 0]  # 疑似阅读行的点，可以标为红色
        self.point1 = XY[label_pred == 1]  # 不知，可以标为绿色
        self.point2 = XY[label_pred == 2]  # 不知，可以标为蓝色

        # 阅读热力图Δx
        self.readPhotoDeltaX = self.deltaX
        self.readPhotoDeltaX /= 1024
        # 计算11点
        self.speedPerPoints = math.floor(len(self.readPhotoDeltaX) / 11)
        if(self.speedPerPoints != 0):
            self.speedPoints = np.empty(shape=(11, 1))  # 待返回数组
            for i in range(11):
                tempSpeed = 0.0
                for j in range(self.speedPerPoints):
                    tempSpeed += self.readPhotoDeltaX[i *
                                                      self.speedPerPoints + j]  # 该范围内速度相加
                tempSpeed /= self.speedPerPoints  # 求平均值
                self.speedPoints[i] = tempSpeed
        else:
            self.speedPoints = -1  # 若数据不足，返回-1
        # 查看书本id和页数
        self.BookIdPageNum = np.empty(shape=(len(self.X), 2), dtype=str)
        # 读取书本id（0位）和页数（1位)
        for i in range(len(self.X)):
            self.BookIdPageNum[i][0] = self.BookId[i]
            self.BookIdPageNum[i][1] = self.PageNum[i]
        # 删除重复元素
        self.BookIdPageNum = np.unique(self.BookIdPageNum, axis=0)
        # 生成图片
        # for i in range(len(self.BookIdPageNum)):
        #     tempPicData = np.zeros((128, 171))  # 热力图数组
        #     for j in range(len(self.X)):
        #         # 判断该点属于哪本书哪一页
        #         if (self.BookId[j] == self.BookIdPageNum[i][0] and self.PageNum[j] == self.BookIdPageNum[i][1]):
        #             # 确定每个点属于的方格
        #             tempPicData[math.floor(self.X[j] / 8)
        #                         ][math.floor(self.Y[j] / 8)] += 1
        #     ax = plt.subplots(figsize=(128, 171))
        #     ax = sns.heatmap(tempPicData, cmap="RdPu", cbar=False)  # 热力图绘制
        #     plt.savefig("../../qReaderData/sightData/" + sys.argv[1] + "ReadPhoto/" + sys.argv[1] +
        #         str(self.BookIdPageNum[i][0]) + str(self.BookIdPageNum[i][1]) + '.jpg',bbox_inches = 'tight')


def main():
    assert 2 == len(sys.argv)
    userId = sys.argv[1]
    analyseJsonPath = "../../qReaderData/sightData/sightAnalyseJson/"
    jsonFilePath = os.path.join(analyseJsonPath, userId, userId + "_Analyse.json")
    with open(jsonFilePath, "w", encoding="utf-8") as f:
        # 4格空格符为格式化标准，以下两行均可用于写入json文件
        initDict = {}
        json.dump(initDict, f, indent=4, ensure_ascii=False)
        # f.write(json.dumps(initDict, indent=4, ensure_ascii=False))
    sightCsvPath = "../../qReaderData/sightData/sightCsv/"
    csvFilePath = os.path.join(sightCsvPath, userId + "_Analyse.csv")
    assert os.path.exists(csvFilePath)
    csv_data = pd.read_csv(csvFilePath)
    assert(len(csv_data))
    res = read(csv_data)
    dict = {"hour": res.hour, "min": res.min, "sec": res.sec, "focus": res.focus,
            "pages": res.Pages, "rows": res.Rows, "speedPoints": res.speedPoints.tolist(),
            "point0Action" : "专注", "point0Color" : "#d71345", "point0" : res.point0[:20].tolist(),
            "point1Action" : "走神", "point1Color" : "#7fb80e", "point1" : res.point1[:20].tolist(),
            "point2Action" : "疑惑", "point2Color" : "#4e72b8", "point2" : res.point2[:20].tolist()}
    with open(jsonFilePath, "w", encoding="utf-8") as f:
        # 4格空格符为格式化标准，以下两行均可用于写入json文件
        # json.dump(dict, f, indent=4, ensure_ascii=False)
        f.write(json.dumps(dict, indent=4, ensure_ascii=False))


if __name__ == "__main__":
    main()



