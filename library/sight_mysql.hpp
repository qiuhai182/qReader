/*
 * @Author: zhangqingjun
 * @Date: 2021-10-27 00:07:58
 * @LastEditTime: 2021-10-27 02:02:00
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: sight_mysql.h
 * @封装sight数据的数据库操作
 */


#pragma once
#include "mysql_tables.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "commentdata.hpp"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <time.h>
#include <vector>
#include <map>

using namespace rapidjson;
using namespace ormpp;
using namespace std;


//数据库读取的字段
struct oneSight
{
  float x;
  float y;
  string timeStamp;
};

struct pageSight
{
  string userId;
  string bookId;
  int pageNum;
  vector<oneSight> sightList;
  string startTime;
  string endTime;
};



/*
 * 客户端提交新视线数据
 * 写入数据库
 */
int insertSightSQL(SightTable &sight)
{ 
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  if (conn->insert(sight) != 1)
  {
    cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
    return 0;
  }
  cout << "视线数据写入SQL" << endl;
  return 1;
}

/*
 * 客户端提交一批次数据的开始时间、结束时间、记录时间
 * 写入数据库
 */
int insertSESQL(ReadSETable &readSE)
{ // 插入视线数据
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  if (conn->insert(readSE) != 1)
  {
    cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
    return 0;
  }

  return 1;
}


//读取sightTable 模糊匹配一天的内的数据 year-month-day
int get_sight_by_timeStamp(vector<SightTable>& result,const string &timeStamp,const string & userId)
{
  auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where timeStamp LIKE \"\%" + timeStamp + "\%\" and userId =  \"" + userId + "\"";
  
  auto res = conn->query<SightTable>(cond);
	if (res.size() == 0)
		return -1;

	for (auto &i : res)
	{
		result.push_back(i);
	}
	return res.size();
}