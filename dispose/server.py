# -*- coding: UTF-8 -*-
# shell内核调用方式： python server.py "userId"

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import sys
import math
import json
from dateutil.parser import parse
import seaborn as sns
import datetime
import copy


def isContinus(t1, t2):
    '''判断两点是否是相邻点，只有连续点才有分析意义'''
    t1 = parse(t1)
    t2 = parse(t2)
    deltaT = abs(t2.second - t1.second)
    if (1 >= deltaT):
        return True
    else:
        return False


def getDeltaTime(t1, t2):
    '''计算两时间差秒数'''
    t1 = parse(t1)
    t2 = parse(t2)
    return (t2 - t1).total_seconds()


def getDeltaDistance(x1, y1, x2, y2):
    '''计算两点距离'''
    return np.sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1))


def getAngle(pA, pB=(0, 0), pC=(1, 0)):
    """
    传入三点坐标，第一、三点分别于中间点连成射线，
    以中间点为两射线交点，返回三点确定两射线夹角，三点不可重复
    传入一点坐标，从原点出发连成射线，返回相对X轴正方向夹角
    """
    if pA == pB or pA == pC:
        return 0
    a = math.sqrt((pB[0]-pC[0])**2+(pB[1]-pC[1])**2)
    b = math.sqrt((pA[0]-pC[0])**2+(pA[1]-pC[1])**2)
    c = math.sqrt((pA[0]-pB[0])**2+(pA[1]-pB[1])**2)
    # abc = (a*a-b*b-c*c)/(-2*b*c)
    # if -1 > abc:
    #     abc = -1
    # elif 1 < abc:
    #     abc = 1
    bca = (b*b-a*a-c*c)/(-2*a*c)
    if -1 > bca:
        bca = -1
    elif 1 < bca:
        bca = 1
    # cba = (c*c-a*a-b*b)/(-2*a*b)
    # if -1 > cba:
        # cba = -1
    # elif 1 < cba:
        # cba = 1
    # A=math.degrees(math.acos(abc))
    # C=math.degrees(math.acos(cba))
    B = math.degrees(math.acos(bca))
    if pB[1] == pC[1]:
        # 求相对x轴正方向角度时有正负值，代表逆时针/顺时针方向
        if pA[1] >= pB[1]:
            return B
        else:
            return -B
    else:
        return B


def getDeltaAngle(pA, pB):
    '''
    传入两个点，以第一个点为原点建立二维xy坐标系，
    选取该坐标系原点x轴正向一点作为第三点，计算原点对应夹角
    '''
    return getAngle(pB, pA, (pA[0] + 1, pA[1]))


def countIndexValue(value, index):
    '''
    传入两个一维数组，分别表示值数组和索引数组
    返回索引指向值之和
    '''
    result = 0.0
    for i in range(len(index)):
        result += value[index[i]]
    return round(result, 2)


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
                getDeltaAngle((x1, y1), (x2, y2)), float))

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



