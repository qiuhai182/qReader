# -*- coding: UTF-8 -*-
# shell内核调用方式： python server.py "xxx/.../x.csv" "xxx/.../x.json"

import os
import sys
import math
import json
import copy
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns; sns.set()
from dateutil.parser import parse
from sklearn.cluster import KMeans, DBSCAN
from matplotlib.pyplot import MultipleLocator
from itertools import cycle  ##python自带的迭代器模块



def isContinus(t1, t2):
    '''判断两点是否在时间上是连续数据'''
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
    传入三点坐标，第一、三点分别与中间点连成射线，以中间点为两射线共同出发点，返回两射线夹角，三点两两之间不可重复
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
    return result


def isContinue(d1, d2):
    '''
    判断传入的两个值是否同正负，认为0与任何数同正负
    '''
    if 0 <= d1 * d2:
        return True
    else:
        return False


class calculateLine:
    '''
    阅读行数计算
    '''
    def __init__(self, csv_data):
        self.allDataLength = len(csv_data)  # 总视线点数
        # 剔除屏幕外点
        csv_data = csv_data[0 < csv_data['x']]
        csv_data = csv_data[csv_data['x'] < 1024]
        csv_data = csv_data[0 < csv_data['y']]
        csv_data = csv_data[csv_data['y'] < 1366]
        # 取x、y、time、pageNum
        self.X = np.array(csv_data['x'])
        self.Y = np.array(csv_data['y'])
        self.PageNum = np.array(csv_data['pageNum'])
        # 倒序剔除连续重复点
        for i in range(len(csv_data) - 1, 0, -1):
            # 相较前一个点，x、y、页数 均未变化，则剔除该点
            if (self.X[i] == self.X[i - 1] and self.Y[i] == self.Y[i - 1] and self.PageNum[i] == self.PageNum[i - 1]):
                csv_data.drop(csv_data.index[[i]], axis=0)  # 剔除连续重复点
        # 重定义X、Y、Time、Page, 新定义BookId
        self.X = np.array(csv_data['x'])
        self.Y = np.array(csv_data['y'])
        self.PageNum = np.array(csv_data['pageNum'])
        self.AllPages = set(self.PageNum)
        self.AllPages = list(self.AllPages)
        self.AllPages = np.array(self.AllPages)
        self.AllData = []
        self.dataNum = len(self.AllPages)
        #将每一页的页码及数据做成2维数组
        for i in range(len(self.AllPages)):
            temp = []
            for j in range(len(self.X)):
                tempXY = [] 
                if self.PageNum[j] == self.AllPages[i]:
                    tempXY.append(self.X[j])
                    tempXY.append(self.Y[j])
                    temp.append(tempXY)
            self.AllData.append(temp)
        self.XWeight = 0.0
        
    def modelLoad(self):
        #模型预加载
        rf = open("../../../dispose/RowTrainResult.json", "r")
        textStr = rf.read()
        dictOfX = json.loads(textStr)
        self.XWeight = dictOfX["XWeight"]
        
    def Row(self, num):
        #加载模型
        self.modelLoad()
        #提取num页的所有点
        self.XYData = np.array(self.AllData[num])
        #计算差值(该数据经过处理放大了1000倍)
        self.XYdifference = np.empty(shape=((len(self.XYData) - 1), 2))
        for i in range(0, len(self.XYData) - 1):
            self.XYdifference[i][0] = self.XYData[i + 1][0] - self.XYData[i][0]
            self.XYdifference[i][1] = self.XYData[i + 1][1] - self.XYData[i][1]
            self.XYdifference[i][1] /= 1366 / 1000
            self.XYdifference[i][0] /= 1024 / 1000
        #self.YRange = np.zeros(shape = (1, 50))
        self.Xdifference = []
        self.rowNum = 0          #计算行数
        self.isBegin = False     #是否开始阅读
        self.addPoint = 0.0       #用户视线向右点相加
        self.minusPoint = 0.0     #用户视线向左点相加
        self.isAddAlready = False #页数是否增加
        #将Δx单独取出放入一个数组
        for i in range (len(self.XYdifference)):
            self.Xdifference.append(self.XYdifference[i][0])
        #计算行数
        for i in range (len(self.Xdifference)):
                #第一个if：判断用户是否开始阅读该页，以x方向速度第一个为正的点开始
                if self.isBegin == False and self.Xdifference[i] < 0:
                    continue
                else:
                    self.isBegin = True
                #第二个if：判断数据为向左还是向右并加入相应的值内
                if self.Xdifference[i] > 0:
                    self.addPoint += self.Xdifference[i]
                else:
                    self.minusPoint += self.Xdifference[i]
                #若还未计入行数(isAddAlready = False)且向右阅读相加的值大于零且向右值（正）加向左值（负)小于阈值可视为视线回转，行数+1，右阅读值清零，
                if (self.isAddAlready == False and self.addPoint > 0 and self.minusPoint + self.addPoint < self.addPoint * self.XWeight):
                    self.rowNum += 1
                    self.minusPoint += self.addPoint * (1-self.XWeight)
                    self.addPoint = 0.0
                    self.isAddAlready = True
                    continue
                #在已经计算行数后且再次向右阅读时将向左值清零并重置isAddAlready
                if self.isAddAlready == True and self.addPoint + self.minusPoint > 0:
                    self.minusPoint = 0.0
                    self.isAddAlready = False  
        #翻页数据没有负数，行数加一
        self.rowNum += 1
        return self.rowNum
    
    def allRows(self):
        #计算所有页行数并返回总值
        self.resultAll = 0
        for i in range(self.dataNum):
            self.resultAll += self.Row(i)
        return self.resultAll

class read:
    '''
    阅读数据分析
    '''
    def Efficient(self):
        return len(self.X) / self.allDataLength;
    def Challange(self, num):
        self.XYData = np.array(self.AllData[num])
        self.db = DBSCAN(eps=70, min_samples=70)
        self.db.fit(self.XYData)
        lables = self.db.labels_
        #Browse:浏览 gaze:注视
        for i in range(len(lables)):
            if lables[i] == -1:
                self.Browse += 1
            else:
                self.Gaze += 1
    def pieChartResult(self):
        for i in range(len(self.AllPages)):
            self.Challange(i)
    def returnBrowse(self):
        return self.Browse / self.allDataLength;
    def returnGaze(self):
        return self.Gaze / self.allDataLength;
    def returnUnfocused(self):
        return 1 - self.Efficient()
    def returnPages(self):
        return len(self.AllPages)
    def __init__(self, csv_data):
        '''初始化'''
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

        self.AllPages = set(self.PageNum)
        self.AllPages = list(self.AllPages)
        self.AllPages = np.array(self.AllPages)
        self.AllData = []
        #将每一页的页码及数据做成2维数组
        for i in range(len(self.AllPages)):
            temp = []
            for j in range(len(self.X)):
                tempXY = [] 
                if self.PageNum[j] == self.AllPages[i]:
                    tempXY.append(self.X[j])
                    tempXY.append(self.Y[j])
                    temp.append(tempXY)
            self.AllData.append(temp)
        #Browse:浏览 gaze:注视    
        self.Browse = 0 
        self.Gaze = 0
        self.pieChartResult()

        # 阅读散点图
        xy = list(zip(self.deltaX, self.deltaY))
        XY = np.array(xy)
        cluster = KMeans(n_clusters=3).fit(XY)  # 构造聚类器
        label_pred = cluster.labels_  # 获取聚类标签
        self.point0 = XY[label_pred == 0]  # 疑似阅读行的点，可以标为红色
        self.point1 = XY[label_pred == 1]  # 不知，可以标为绿色
        self.point2 = XY[label_pred == 2]  # 不知，可以标为蓝色

        # 阅读速度
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
            self.speedPoints = np.empty((11, 1))  # 若数据不足


        # # 阅读热力图
        # self.readPoint = np.zeros((128, 171))
        # for i in range(len(self.X)):
        #     self.readPoint[math.floor(self.X[i] / 8)][math.floor(self.Y[i] / 8)] += 1
        # ax = plt.subplots(figsize=(128, 171))
        # plt.axis('off')
        # ax = sns.heatmap(self.readPoint, cmap="RdPu", cbar=False)  # 热力图绘制
        # plt.savefig("../../../../qReaderData/sightData/sightAnalyseJson/" + sys.argv[1] + '/' + sys.argv[1] + '_Analyse.png', bbox_inches = 'tight')


def main():
    assert 3 == len(sys.argv)
    csvFilePath = sys.argv[1]
    assert os.path.exists(csvFilePath)
    outputJsonPath = sys.argv[2]
    with open(outputJsonPath, "w", encoding="utf-8") as f:
        # 清空文件内容、格式化，4格空格符为格式化标准，以下两行均可用于写入json文件
        initDict = {}
        json.dump(initDict, f, indent=4, ensure_ascii=False)
        # f.write(json.dumps(initDict, indent=4, ensure_ascii=False))
    sightCsvPath = "../../../../qReaderData/sightData/sightCsv/"
    csv_data = pd.read_csv(csvFilePath)
    assert(len(csv_data))
    resLine = calculateLine(copy.deepcopy(csv_data))
    res = read(copy.deepcopy(csv_data))
    dict = {"hour": res.hour, "min": res.min, "sec": res.sec, "focus": res.Efficient(),
            "pages": res.returnPages(), "rows": resLine.allRows(), "speedPoints": res.speedPoints.tolist(),
            "Browse" : "浏览比重", "BrowsePercentage":res.returnBrowse(),
            "Gaze" : "注视比重", "GazePercentage":res.returnGaze(),
            "Unfocused" : "不专注比重", "UnfocusedPercentage": res.returnUnfocused()}
    with open(outputJsonPath, "w", encoding="utf-8") as f:
        # 4格空格符为格式化标准，以下两行均可用于写入json文件
        # json.dump(dict, f, indent=4, ensure_ascii=False)
        f.write(json.dumps(dict, indent=4, ensure_ascii=False))


if __name__ == "__main__":
    main()



