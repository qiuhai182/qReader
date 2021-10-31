'''
Author: your name
Date: 2021-10-27 02:21:43
LastEditTime: 2021-10-27 02:24:34
LastEditors: your name
Description: In User Settings Edit
FilePath: qReader/accountService/split.py
'''
# -*- coding: UTF-8 -*-
import os
import subprocess
from PyPDF4 import PdfFileWriter, PdfFileReader
import shutil


#将pdf分片，用文件哈希码作为前缀标识书本
def split_pdf(filename, filepath, save_dirpath, hashcode,step = 1):
    #创建文件夹
    if not os.path.exists(save_dirpath):
        os.mkdir(save_dirpath)
    # 打开原书籍
    open_pdf_file = open(filepath,'rb')
    # 读取pdf
    inputpdf = PdfFileReader(open_pdf_file)
    # pdf页数
    pages = inputpdf.numPages
    oldPageIndex = 0    #旧pdf文件页下标
    newPdfCount = 1     #分割pdf数目
    while(oldPageIndex < pages):
        inputpdf = PdfFileReader(open_pdf_file)
        output =  PdfFileWriter()
        newPage = 0     #新pdf文件页下标
        while(newPage < step and oldPageIndex < pages  ):
            output.addPage(inputpdf.getPage(oldPageIndex))
            oldPageIndex += 1 
            newPage += 1
        # new_file_path = save_dirpath + (os.path.splitext(filename))[0] + "_" + str(newPdfCount -1 )+ ".pdf"
        new_file_path = save_dirpath + hashCode + "_" + str(newPdfCount -1 )+ ".pdf"
        newPdfCount +=1
        with open(new_file_path, "wb") as outputStream:
            output.write(outputStream)
    open_pdf_file.close() 
#函数结束


filename = "操作系统设计与实现-高教出版社-哈工大李治军.pdf"
exeChmod = "sha1sum ../../qReaderData/books/wholeBooks/" + filename 
byteResult =subprocess.run(exeChmod,shell=True ,stdout=subprocess.PIPE).stdout
strResult = str(byteResult)[2:-3]
print(strResult)
# 获哈希码
hashCode = (strResult.split())[0]
# 保存简略信息，利于数据库插入
wholeFilesPath = "../../qReaderData/books/wholeBooks/"
hashRenamePath = "../../qReaderData/books/hashRenameBooks/"
with  open('../../qReaderData/book_Inf.txt','a+') as f:
    f.write(filename + "   ")
    f.write(hashCode + "  \n")
    f.close()
print("pdf文件 {} 通过sha1sum算法得到文件哈希码为:{} ".format(filename,hashCode))
filepath = wholeFilesPath + filename
save_dirpath = hashRenamePath + "/"
rename_pdf_path = save_dirpath + hashCode + ".pdf"
if not os.path.exists(save_dirpath):
    os.mkdir(save_dirpath)
shutil.copy(filepath, rename_pdf_path) # 复制文件
#二进制存入mysql


