import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from itertools import cycle  ##python自带的迭代器模块
import math
import copy
from matplotlib.pyplot import MultipleLocator
import os
import json
import sys

class calculateLine:
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
def main(CsvName, outputName):
    assert os.path.exists(CsvName)
    csv_data = pd.read_csv(CsvName)
    assert(len(csv_data))
    res = calculateLine(csv_data)
    loadDict = {"rows" : res.allRows()}
    #assert os.path.exists("Analyse.json")
    with open(outputName, "w", encoding="utf-8") as f:
        # 4格空格符为格式化标准，一下两行均可用于写入json文件
        # f.write(json.dumps(dict, indent=4))
        json.dump(loadDict, f, indent=4)

if __name__=="__main__":
    if len(sys.argv) == 1:
        name = "../oxc_Train.csv"
        output = "Analyse.json"
        main(name, output)
    elif len(sys.argv) > 3:
        print("usage: ./xxx.py (csvFine) (outputFile)")
    else:
        name = sys.argv[1]
        output = sys.argv[2]
        main(name, output)
