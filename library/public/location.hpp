#pragma once

#ifndef _LOCATION_H
#define _LOCATION_H

#include <iostream>
#include <thread>
#include <memory>
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"
using namespace ormpp;
using namespace std;

enum class RES_CODE{
    FAL = 0,
    SUS = 1
};


DEFINE_string(dataRoot, "../../../../", "数据存储目录所在根目录");
// DEFINE_string(dataRoot, "../../../../", "数据存储目录所在根目录");
DEFINE_string(dataPath, FLAGS_dataRoot + "qReaderData/", "数据存储目录");
DEFINE_string(commentPath, FLAGS_dataPath + "commentData/", "评论数据存储目录");
DEFINE_string(commentJson, FLAGS_commentPath + "commentJson/", "评论数据json文件存储目录");
DEFINE_string(commentCsv, FLAGS_commentPath + "commentCsv/", "评论数据csv文件存储目录");
DEFINE_string(sightPath, FLAGS_dataPath + "sightData/", "视线数据存储目录");
DEFINE_string(sightJson, FLAGS_sightPath + "sightJson/", "视线数据json文件存储目录");
DEFINE_string(sightCsv, FLAGS_sightPath + "sightCsv/", "视线数据csv文件存储目录");
DEFINE_string(imagesPath, FLAGS_dataPath + "images/", "图片存储文件夹");
DEFINE_string(booksPath, FLAGS_dataPath + "books/", "pdf书籍存放目录");
DEFINE_string(wholeBooksPath, FLAGS_booksPath + "wholeBooks/", "未分割pdf存放目录");
DEFINE_string(pdfPath, FLAGS_booksPath + "spiltedBooks/", "整本书重命名为书籍ID的复制pdf、分割后pdf的存储文件夹");
DEFINE_string(sightAnalyseJson, FLAGS_dataPath + "sightAnalyseJson/", "数据分析json文件存放目录");
DEFINE_string(renameBooksPath, FLAGS_dataPath + "books/hashRenameBooks/", "数据分析json文件存放目录");


DEFINE_string(service_ip, "192.168.91.128", "用于文件下载的ip外网地址");
DEFINE_string(filePort, FLAGS_service_ip + ":" + "8006", "下载服务服务ip:port");
DEFINE_string(bookHeadUrlPre, "http://" + FLAGS_filePort + "/fileService/fileDownFun/books/bookCover/", "书籍封面下载url前缀");
DEFINE_string(bookBodyUrlPre, "http://" + FLAGS_filePort + "/fileService/fileDownFun/books/hashRenameBooks/", "整本书籍下载url前缀");
DEFINE_string(bookBodyPathPre, "http://" + FLAGS_filePort + "/fileService/fileDownFun/books/hashRenameBooks/", "整本书籍路径前缀");

#endif


