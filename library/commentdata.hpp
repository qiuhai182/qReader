#pragma once

#include "mysql_tables.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "contenttype.hpp"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <cstring>
#include <vector>
using namespace rapidjson;
using namespace ormpp;
using namespace std;
const int maxLength = 1024; // 某些数据的最大长度

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

/*url在运行时生成*/
// DEFINE_string(runIp, "39.105.217.90", "运行时IP");
// DEFINE_string(fileUpDownPort, "8006", "运行时文件下载服务端口");
// DEFINE_string(url, "http://" + FLAGS_runIp + ":" + FLAGS_fileUpDownPort + ":fileService/fileDownFun/", "文件下载url前缀");


    // 封装评论信息数据库操作、json格式存取评论

    /*
 * 
 * 分割string
 */
    vector<string> splitString(string &str, char tag)
{
  vector<string> listString;
  string subStr;
  //遍历字符串，同时将i位置的字符放入到子串中，当遇到tag（需要切割的字符时）完成一次切割
  //遍历结束之后即可得到切割后的字符串数组
  for (size_t i = 0; i < str.length(); i++)
  {
    if (tag == str[i]) //完成一次切割
    {
      if (!subStr.empty())
      {
        listString.push_back(subStr);
        subStr.clear();
      }
    }
    else //将i位置的字符放入子串
    {
      subStr.push_back(str[i]);
    }
  }
  if (!subStr.empty()) //剩余的子串作为最后的子字符串
  {
    listString.push_back(subStr);
  }
  return listString;
}

/*
 * 
 * 查找字符出现位置
 */
vector<int> findFlag(string &str, char ch)
{
  vector<int> res;
  for (int i = 0; i < str.length(); ++i)
  {
    char chi = str.substr(i, 1).data()[0];
    if (chi == ch)
    {
      res.push_back(i);
    }
  }
  return res;
}

/*
 * 
 * 根据给定路径，确保其最终存在
 */
int configurePath(string filePath)
{
  string pwdFile(__FILE__);                    // 当前文件路径
  string fileName = basename(filePath.data()); // 文件名
  string preFolder;                            // 父目录
  size_t lastFlag = filePath.rfind('/');
  filePath.erase(lastFlag + 1);
  if (NULL == opendir(filePath.c_str()))
  {
    // filePath不存在,递归父目录,创建filePath
    vector<int> flags = findFlag(filePath, '/');
    // 获取'/'的索引
    int lastOneFlag = flags[flags.size() - 1];                                      // 倒数第1个'/'的位置
    int lastTwoFlag = flags[flags.size() - 2];                                      // 倒数第2个'/'的位置
    string curFolder = filePath.substr(lastTwoFlag + 1, lastOneFlag - lastTwoFlag); // 存储当前最细枝目录
    preFolder = filePath.erase(lastTwoFlag + 1);                                    // filePath删除当前目录得到上一级目录
    filePath += curFolder;                                                          // filePath恢复当前目录
    if (FLAGS_dataRoot == preFolder || 0 == configurePath(preFolder))
    {
      // 父目录存在 或 递归创建父目录成功
      if (NULL == opendir(filePath.data()))
      {
        // filePath不存在,创建filePath
        int ret = mkdir(filePath.data(), 0775); // 权限，0775 与 775 不等同
        if (ret)
        {
          cout << "FILE: " << __FILE__ << endl
               << "创建(" << filePath << ")目录错误: " << strerror(errno) << " LINE  "
               << __LINE__ << endl;
          return -1;
        }
        return 0;
      }
      else
      {
        // filePath已存在
        return 0;
      }
    }
  }
  else
  {
    // filePath已存在
    return 0;
  }
}

/*
 * 
 * 获得对应文件命名标志
 */
string getCommentFileFlag(const CommentTable &comment)
{
  if (0 != comment.pageNum)
  {
    return comment.bookId + "_" + to_string(comment.pageNum); // 一页的评论使用一个json文件
  }
  else
  {
    return comment.bookId; // 对一本书的评论使用一个json文件
  }
}

/*
 * json键值
 * 对应的唯一标识id字符串(bookId_pageNum_commentId)
 */
string getFlagId(const CommentTable &comment)
{
  return (comment.bookId + "&" + to_string(comment.pageNum) + "&" +
          to_string(comment.commentId));
}

/*
 * json键值
 * 对应的唯一标识id字符串(bookId_pageNum_commentId)
 */
string getParentFlagId(const CommentTable &comment)
{
  return (comment.bookId + "&" + to_string(comment.pageNum) + "&" +
          to_string(comment.parentId));
}

/*
 * 
 * 获得对应json文件的存放路径
 */
string getCommentBookIdPath(const CommentTable &comment)
{
  return (FLAGS_commentJson + comment.bookId + "/");
}

/*
 * 
 * 获得对应json文件 路径+文件名
 */
string getCommentJsonPath(const CommentTable &comment, string &filePath)
{
  return (filePath = getCommentBookIdPath(comment) +
                     getCommentFileFlag(comment) +
                     ".json"); // 根据comment获得对应json文件访问路径
}

/*
 * 
 * 获得对应csv文件 路径+文件名
 */
string getCommentCsvPath(const CommentTable &comment, string &filePath)
{
  return (filePath = FLAGS_commentCsv + comment.bookId + "_Comment.csv");
}

/*
 * 根据DOM --writer--> string 和 filePath
 * 写进json格式的数据
 */
int writeCommentJsonDoc(const CommentTable &comment, Document &document)
{
  string filePath;
  getCommentJsonPath(comment, filePath);
  FILE *fp = fopen(filePath.data(), "wb");
  char writeBuffer[65535];
  FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
  Writer<FileWriteStream> writer(os);
  document.Accept(writer);
  fclose(fp);

  return 1;
}

/*
 * 
 * 确保commentJson文件存在
 */
int configureCommentJson(const CommentTable &comment)
{
  string jsonFile;
  getCommentJsonPath(comment, jsonFile);
  configurePath(jsonFile);
  ifstream userInFile(jsonFile, ios::in | ios::binary);
  if (!userInFile.good())
  { // 文件读取失败
    Document document;
    document.Parse("{}");
    writeCommentJsonDoc(comment, document); // 创建目录及json文件
    return 0;
  }
  userInFile.close();
  return 0;
}

/*
 * 
 * 确保commentCsv文件存在
 */
int configureCommentCsv(const CommentTable &comment)
{
  string csvFile;
  getCommentCsvPath(comment, csvFile);
  configurePath(csvFile);
  ifstream userInFile(csvFile, ios::in | ios::binary);
  if (!userInFile.good())
  { // 文件读取失败
    ofstream userOutFile(csvFile, ios::out | ios::binary);
    if (!userOutFile.good())
    {
      cout << "create " << csvFile << " failed" << endl;
      userInFile.close();
      return -1;
    }
    else
    {
      userOutFile << "x" << ',' << "y" << ',' << "time" << ','
                  << "startTime" << ',' << "endTime" << ','
                  << "bookId" << ',' << "pageNum" << ','
                  << "userId" << endl;
      userInFile.close();
      userOutFile.close();
      return 0;
    }
  }
  userInFile.close();
  return 0;
}

/*
 * 初始化函数
 * 初始化数据目录，确保其存在
 */
int initDataPath()
{
  initContentType(); // 初始化contentType
  configurePath(FLAGS_sightAnalyseJson);
  configurePath(FLAGS_commentCsv);
  configurePath(FLAGS_commentJson);
  configurePath(FLAGS_sightCsv);
  configurePath(FLAGS_sightJson);
  configurePath(FLAGS_imagesPath);
  configurePath(FLAGS_wholeBooksPath);
  configurePath(FLAGS_pdfPath);
  return 0;
}

/*
 * 根据时间格式：xxxx-xx-xx xx:xx:xx
 * 得到int时间戳
 */
long getTimeStamp(char *timeString)
{
  int iY = atoi(timeString); // atoi函数遇到非数字自动结束取位
  int iM = atoi(timeString + 5);
  int iD = atoi(timeString + 8);
  int iH = atoi(timeString + 11);
  int iMin = atoi(timeString + 14);
  int iS = atoi(timeString + 17);
  struct tm timeInfo;
  memset(&timeInfo, 0, sizeof(timeInfo));
  timeInfo.tm_year = iY - 1900;
  timeInfo.tm_mon = iM - 1;
  timeInfo.tm_mday = iD;
  timeInfo.tm_hour = iH;
  timeInfo.tm_min = iMin;
  timeInfo.tm_sec = iS;
  return mktime(&timeInfo);
}

/*
 * 根据时间格式：xxxx-xx-xx xx:xx:xx
 * 得到int时间戳
 */
long getTimeStamp(const char *timeString)
{
  return getTimeStamp(const_cast<char *>(timeString));
}

/*
 * 根据时间格式：xxxx-xx-xx xx:xx:xx
 * 得到int时间戳
 */
long getTimeStamp(const string &timeString)
{
  char timeChar[maxLength];
  for (int i = 0; i < timeString.size(); ++i)
  {
    timeChar[i] = timeString.data()[i];
  }
  return getTimeStamp(timeChar);
}

/*
 * 根据时间格式：xxxx-xx-xx xx:xx:xx
 * 得到int时间戳
 */
long getTimeStamp(string &timeString)
{
  return getTimeStamp(timeString.data());
}

/*
 * 判断收到的时间是否是日期格式
 * 是则返回True
 * 若是时间戳则返回False
 */
bool isStampTime(string recvTime)
{
  string id = recvTime.substr(4, 1);
  return "-" == id;
}

/*
 * 根据int时间戳
 * 得到string时间
 */
string getStampTime(int timeStamp)
{
  char res[1024];
  time_t tick = (time_t)timeStamp;
  struct tm timeInfo = *localtime(&tick);
  strftime(res, sizeof(res), "%Y-%m-%d %H:%M:%S", (struct tm *)&timeInfo);
  return res;
}

/*
 * 初始化document
 * 不存在csv就创建，存在csv则读取
 */
int initDocument(const CommentTable &comment, Document &document)
{
  string filePath;
  getCommentJsonPath(comment, filePath);
  ifstream in(filePath.data());
  if (!in.is_open())
  {
    configureCommentJson(comment);
  }
  in.open(filePath.c_str()); // 读取json数据
  if (!in.is_open())
  {
    cout << "FILE:" << __FILE__ << " "
         << "read " << filePath << " failed"
         << " LINE" << __LINE__ << endl;
    return -1;
  }
  istreambuf_iterator<char> begin(in);
  istreambuf_iterator<char> end;
  string json(begin, end);
  document.Parse(json.data());
  in.close();
  return 0;
}

/*
 * 
 * 初始化document
 */
int initDocument(const CommentTable &comment, Document *document)
{
  return initDocument(comment, *document);
}

/*
 * 
 * 读取json文件内容
 */
int getCommenDocString(const CommentTable &comment, Document &document,
                       string &result)
{
  StringBuffer sbuf;
  Writer<StringBuffer> writer(sbuf);
  document.Accept(writer);
  result = sbuf.GetString();
  return 1;
}

/*
 * 
 * 读取json文件内容
 */
int getCommenJsonString(const CommentTable &comment, string &result)
{
  Document document;
  getCommenDocString(comment, document, result);
  return 1;
}

/*
 *
 *
 */
int get_max_commentid()
{ // 查询最新可用评论id,评论数不会超过int32的最大值2147483647
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  string cond = "select max(commentId) from CommentTable";
  auto res = conn->query<std::tuple<int>>(cond);
  if (res.size() != 1)
  {
    return -1;
  }
  int maxnum = std::get<0>(res[0]) + 1;
  return maxnum;
}

/*
 * 根据DOM --writer--> string 和 filePath
 * 写json格式的数据
 */
int writeCommentJsonString(const string &commonJson, const char *filePath)
{
  Document document;
  document.Parse(commonJson.c_str());
  FILE *fp = nullptr;
  if (!(fp = fopen(filePath, "wb+")))
  {
    cout << "FILE: " << __FILE__ << " "
         << "写入json时，打开文件失败"
         << " LINE " << __LINE__ << endl;
    return -1;
  }
  char writeBuffer[65535];
  FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
  Writer<FileWriteStream> writer(os);
  document.Accept(writer);
  fclose(fp);
  return 1;
}

/*
 * 根据新评论数据获取到 已存在的/新建的 document
 * 写入新评论到json文件
 */
int writeTopComment(CommentTable &comment, Document *document)
{
  // 属性写入csv文件
  ofstream outFile;
  string csvFile;
  getCommentCsvPath(comment, csvFile);
  outFile.open(csvFile, ios::app | ios::binary);
  if (!outFile.good())
  {
    cout << "续写csv失败" << endl;
  }
  string realTime = comment.timeStamp;  // 收到的时间，可能为日期格式，也可能为时间戳
  string sign_ = realTime.substr(4, 1); // 取第5位字符，检查是否是符号'-'以判断是时间戳还是日期格式
  if ("-" != sign_)
  {                                                 // 时间戳格式
    realTime = getStampTime(atoi(realTime.data())); // 真实日期 xxxx-xx-xx xx:xx:xx
  }
  outFile << comment.commentId << ',' << comment.bookId.data() << ','
          << comment.pageNum << ',' << comment.parentId << ','
          << comment.userId.data() << ','
          << realTime << ','
          << comment.content.data() << ',' << comment.hitId.data() << endl;
  cout << "评价写入csv" << endl;
  // 属性写入document
  Document::AllocatorType &allocator = document->GetAllocator();
  string bufString;
  Value commentId;              // 评论信息id
  Value bookId(kStringType);    // 书籍id
  Value pageNum;                // 页数
  Value parentId;               // 评论对象id(只有页数存在时有意义)
  Value userId(kStringType);    // 用户id
  Value timeStamp(kStringType); // 时间
  Value content(kStringType);   // 内容
  Value hitId(kStringType);     // 点赞id
  // 为评论属性赋值
  commentId.SetInt(comment.commentId);
  bookId.SetString(comment.bookId.data(), allocator);
  pageNum.SetInt(comment.pageNum);
  parentId.SetInt(comment.parentId);
  userId.SetString(comment.userId.data(), allocator);
  timeStamp.SetString(realTime.data(), allocator);
  content.SetString(comment.content.data(), allocator);
  hitId.SetString(comment.hitId.data(), allocator);
  // 属性写入document
  if (0 != comment.pageNum)
  {
    // 页评论
    // 判断是否是第一条评论
    bufString = "page" + to_string(comment.pageNum);
    char pagenum[maxLength];
    strcpy(pagenum, bufString.data());
    if (!((*document).HasMember(pagenum)))
    { // 没有页评论对象
      Value pageNumObject(kObjectType);
      document->AddMember(StringRef(pagenum), pageNumObject,
                          allocator); // 添加页评论对象
    }
    // 判断评论id是否冲突
    bufString = getFlagId(comment);
    char commentid[maxLength];
    strcpy(commentid, bufString.c_str());
    if ((*document)[pagenum].HasMember(commentid))
    { // 页评论对象有该评论子对象
      cout << "FILE: " << __FILE__ << " "
           << "评论ID(commentId= " << commentid << ")冲突，写入json失败"
           << " LINE  " << __LINE__ << endl;
      return 0;
    }
    Value commentObject(kObjectType);
    (*document)[pagenum].AddMember(StringRef(commentid), commentObject,
                                   allocator); // 添加页评论子对象
    // 添加属性
    (*document)[pagenum][commentid].AddMember("commentId", commentId,
                                              allocator); // 添加评论id属性
    (*document)[pagenum][commentid].AddMember("bookId", bookId,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("pageNum", pageNum,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("parentId", parentId,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("userId", userId,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("timeStamp", timeStamp,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("content", content,
                                              allocator); // 添加书籍id属性
    (*document)[pagenum][commentid].AddMember("hitId", hitId,
                                              allocator); // 添加书籍id属性
    // 写入json文件
    writeCommentJsonDoc(comment, *document);
    if ((*document)[pagenum].HasMember(commentid))
    { // 判读写入成功与否
      cout << "评价写入json" << endl;
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    // 书籍评论
    // 判断是否是第一条评论
    bufString = "book" + comment.bookId;
    char bookflag[maxLength];
    strcpy(bookflag, bufString.data());
    if (!((*document).HasMember(bookflag)))
    { // 没有书籍评论对象
      Value pageNumObject(kObjectType);
      document->AddMember(StringRef(bookflag), pageNumObject,
                          allocator); // 添加书籍评论对象
    }
    // 判断评论id是否冲突
    bufString = getFlagId(comment);
    char commentid[maxLength];
    strcpy(commentid, bufString.c_str());
    if (document->HasMember(commentid))
    { // 书籍评论对象有该评论子对象
      cout << "FILE: " << __FILE__ << " "
           << "评论ID(commentId= " << commentid << ")冲突，写入json失败"
           << " LINE  " << __LINE__ << endl;
      return 0;
    }
    Value commentObject(kObjectType);
    (*document)[bookflag].AddMember(StringRef(commentid), commentObject,
                                    allocator); // 添加书籍评论
    // 添加属性
    (*document)[bookflag][commentid].AddMember("commentId", commentId,
                                               allocator); // 添加评论id属性
    (*document)[bookflag][commentid].AddMember("bookId", bookId,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("pageNum", pageNum,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("parentId", parentId,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("userId", userId,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("timeStamp", timeStamp,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("content", content,
                                               allocator); // 添加书籍id属性
    (*document)[bookflag][commentid].AddMember("hitId", hitId,
                                               allocator); // 添加书籍id属性
    // 写入json文件
    writeCommentJsonDoc(comment, *document);
    if ((*document)[bookflag].HasMember(commentid))
    { // 判读写入成功与否
      cout << "评价写入json" << endl;
      return 1;
    }
    else
    {
      return 0;
    }
  }
  return 1;
}

/*
 * 根据新评论数据获取到 已存在的/新建的 document
 * 写入回复评论到json文件
 */
int writeReplyComment(CommentTable &comment, Document *document)
{
  Document::AllocatorType &allocator = document->GetAllocator();
  string bufString;
  Value commentId;              // 评论信息id
  Value bookId(kStringType);    // 书籍id
  Value pageNum;                // 页数
  Value parentId;               // 评论对象id(只有页数存在时有意义)
  Value userId(kStringType);    // 用户id
  Value timeStamp(kStringType); // 时间
  Value content(kStringType);   // 内容
  Value hitId(kStringType);     // 点赞id
  // 为评论属性赋值
  commentId.SetInt(comment.commentId);
  bookId.SetString(comment.bookId.data(), allocator);
  pageNum.SetInt(comment.pageNum);
  parentId.SetInt(comment.parentId);
  userId.SetString(comment.userId.data(), allocator);
  timeStamp.SetString(comment.timeStamp.data(), allocator);
  content.SetString(comment.content.data(), allocator);
  hitId.SetString(comment.hitId.data(), allocator);
  // 属性添加到document
  char parentid[maxLength];
  if (0 != comment.pageNum)
  {
    // 页评论
    // 存储页标志
    bufString = "page" + to_string(comment.pageNum);
    char pagenum[maxLength];
    strcpy(pagenum, bufString.data());
    // 判断父节点是否存在
    bufString = getParentFlagId(comment);
    strcpy(parentid, bufString.data());
    if (!(*document)[pagenum].HasMember(parentid))
    { // 没有对应的顶层评论
      cout << "FILE: " << __FILE__ << " "
           << "评论父节点不存在，写入json失败"
           << " LINE  " << __LINE__ << endl;
      return -1;
    }
  }
  else
  {
    // 书籍评论
    // 存储书籍标志
    bufString = "book" + comment.bookId;
    char bookflag[maxLength];
    strcpy(bookflag, bufString.data());
    // 判断父节点是否存在
    bufString = getParentFlagId(comment);
    strcpy(parentid, bufString.data());
    if (!(*document)[bookflag].HasMember(parentid))
    { // 没有对应的顶层评论
      cout << "FILE: " << __FILE__ << " "
           << "评论父节点不存在，写入json失败"
           << " LINE  " << __LINE__ << endl;
      return -1;
    }
  }
  // 以下添加到document、写入json
  // 判断回回复存储对象是否存在
  if (!((*document).HasMember(parentid)))
  { // 没有父评论对象
    Value parentObject(kObjectType);
    document->AddMember(StringRef(parentid), parentObject,
                        allocator); // 添加父评论对象
  }
  // 判断评论id是否冲突
  bufString = getFlagId(comment);
  char commentid[maxLength];
  strcpy(commentid, bufString.c_str());
  if ((*document)[parentid].HasMember(commentid))
  { // 页评论对象有该评论子对象
    cout << "FILE: " << __FILE__ << " "
         << "评论ID(commentId= " << commentid << ")冲突，写入json失败"
         << " LINE  " << __LINE__ << endl;
    return 0;
  }
  // 评论子对象
  Value commentObject(kObjectType);
  (*document)[parentid].AddMember(StringRef(commentid), commentObject,
                                  allocator); // 添加回复评论子对象
  // 添加属性
  (*document)[parentid][commentid].AddMember("commentId", commentId,
                                             allocator); // 添加评论id属性
  (*document)[parentid][commentid].AddMember("bookId", bookId,
                                             allocator); // 添加书籍id属性
  (*document)[parentid][commentid].AddMember("parentId", parentId,
                                             allocator); // 添加书籍id属性
  (*document)[parentid][commentid].AddMember("userId", userId,
                                             allocator); // 添加书籍id属性
  (*document)[parentid][commentid].AddMember("timeStamp", timeStamp,
                                             allocator); // 添加书籍id属性
  (*document)[parentid][commentid].AddMember("content", content,
                                             allocator); // 添加书籍id属性
  (*document)[parentid][commentid].AddMember("hitId", hitId,
                                             allocator); // 添加书籍id属性
  // 写入json文件
  writeCommentJsonDoc(comment, *document);
  if ((*document)[parentid].HasMember(commentid))
  { // 判读写入成功与否
    cout << "回复写入json" << endl;
    return 1;
  }
  else
  {
    return 0;
  }
  return 1;
}

/*
 * 客户端提交新评论数据
 * 写入数据库
 */
int insertCommentSQL(CommentTable &comment)
{ // 插入评论数据
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  string realTime = comment.timeStamp;  // 收到的时间，可能为日期格式，也可能为时间戳
  string sign_ = realTime.substr(4, 1); // 取第5位字符，检查是否是符号'-'以判断是时间戳还是日期格式
  if ("-" != sign_)
  {                                                          // 时间戳格式
    comment.timeStamp = getStampTime(atoi(realTime.data())); // 真实日期 xxxx-xx-xx xx:xx:xx
  }
  if (conn->insert(comment) != 1)
  {
    cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
    return 0;
  }
  cout << "评价写入SQL" << endl;
  return 1;
}

/*
 *
 *
 */
int del_comment(const int &comment_id)
{ // 删除评论数据
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  string cond = "commentId = " + to_string(comment_id); // ormpp删除不可用 where
  if (conn->delete_records<CommentTable>(cond))
    return 1;
  else
    return -1;
}

/*
 *
 *
 */
int get_user_comment(const string &userId,
                     vector<CommentTable> &res)
{ // 用户查询自己的评论记录
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  string cond = "where userId =" + userId;
  res = conn->query<CommentTable>(cond);
  return res.size();
}

/*
 * 查询某本书的整体评价
 *
 */
int get_book_comment(const string &bookId,
                     vector<CommentTable> &res)
{ // 查询某本书的评价
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  auto result = conn->query<std::tuple<int, string, int, int, string, string,
                                       string, string, string, string>>(
      "select "
      "commentId,bookId,pageNum,parentId,timeStamp,content,hitId,userId,"
      "userName,userHead "
      "from CommentTable where pageNum = '0' and bookId = '" +
      bookId + "'");
  for (auto &oneres : result)
  {
    CommentTable *comment = new CommentTable{
        std::get<0>(oneres), std::get<1>(oneres), std::get<2>(oneres),
        std::get<3>(oneres), std::get<4>(oneres), std::get<5>(oneres),
        std::get<6>(oneres), std::get<7>(oneres), std::get<8>(oneres),
        std::get<9>(oneres)};
    res.push_back(*comment);
  }
  return res.size();
}

/*
 *
 *
 */
int get_page_comment(const string &bookId, const int &pageNum,
                     vector<CommentTable> &res)
{ // 获得某书，某页的评论信息
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  auto result = conn->query<std::tuple<int, string, int, int, string, string,
                                       string, string, string, string>>(
      "select "
      "commentId,bookId,pageNum,parentId,timeStamp,content,hitId,userId,"
      "userName,userHead "
      "from CommentTable where bookId = '" +
      bookId + "' and pageNum = '" + to_string(pageNum) + "'");
  for (auto &oneres : result)
  {
    CommentTable *comment = new CommentTable{
        std::get<0>(oneres), std::get<1>(oneres), std::get<2>(oneres),
        std::get<3>(oneres), std::get<4>(oneres), std::get<5>(oneres),
        std::get<6>(oneres), std::get<7>(oneres), std::get<8>(oneres),
        std::get<9>(oneres)};
    res.push_back(*comment);
  }
  return res.size();
}

/*
 * 客户端提交新评论数据
 * 分类写入json文件
 */
int insertCommentData(CommentTable &comment)
{
  configureCommentJson(comment);
  configureCommentCsv(comment);
  Document *document = new Document();
  initDocument(comment, document);
  if (0 != comment.parentId)
  {
    return writeReplyComment(comment, document); // 回复评论写入json
  }
  else
  {
    return writeTopComment(comment, document); // 直接评论写入json
  }
  return 1;
}
