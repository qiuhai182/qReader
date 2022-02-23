#pragma once

#ifndef _BOOKDOWNLOADCOUNT_H
#define _BOOKDOWNLOADCOUNT_H

#include <iostream>
#include <thread>
#include <map>
#include <memory>
#include <typeinfo>
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"
#include "sql_pool.hpp"

using namespace ormpp;
using namespace std;
typedef string option;

namespace ormpp
{

    /** 
     * 书籍下载阅读次数统计
     */
    struct BookDownloadCountTable
	{ // 搜索书籍次数统计
		int autoBookId;         // 外键
        string bookId;          // 书籍Id
		int times; 				// 下载次数
		string dayTime;			// 数据记录日期，精确到天 
		string bookName;		// 书籍name
	};
    REFLECTION(BookDownloadCountTable, autoBookId, bookId, times, dayTime, bookName);

    class BookDownloadCount{
    public: 
        BookDownloadCount(const bool create_status = true){
            __isCreate = create_status;
			if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create BookDownloadCountTable error ";
			}
        }
        SQL_STATUS set_newest_count(const int &autoBookId, const string &bookId, const int &times, const string &dayTime, const string &bookName);
        SQL_STATUS set_newest_count(const BookDownloadCountTable &downloadCount);
		SQL_STATUS get_newest_count_by_id(BookDownloadCountTable &downloadCount, const string &bookId);

    private:
		SQL_STATUS create_table();
        bool __isCreate;
    };
}

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍下载统计@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/

SQL_STATUS BookDownloadCount::create_table()
{
    /*创建书籍搜索信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    conn->execute("DROP TABLE IF EXISTS BookDownloadCountTable");
    string state = 
    "CREATE TABLE BookDownloadCountTable( "
        " autoBookId INTEGER NOT NULL, "
        " bookId text NOT NULL , "
        " times INTEGER NOT NULL DEFAULT 0 , "
        " dayTime text NOT NULL , "
        " bookName text NOT NULL, "
        " PRIMARY KEY (autoBookId) ,"
        " CONSTRAINT downloadcount_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB3 " ;
    // insert into BookDownloadCountTable (autobookid, bookid, dayTime, times, bookName) values (10000011, "e3916ccea0066aebb38babb547ebde274dd1803e", "2022-02-23", 1, "C++算法刷题必看");
    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}


SQL_STATUS BookDownloadCount::set_newest_count(const BookDownloadCountTable &downloadCount)
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = "where autoBookId = \'" + to_string(downloadCount.autoBookId) + "\'";
	auto res = conn->query<BookDownloadCountTable>(cond);
    if(0 == res.size())
    {
        cond = "update BookDownloadCountTable set times = " + to_string(downloadCount.times) + " where bookId = " + downloadCount.bookId;
        return execute_sql(conn, "update download times", cond);
    }
    else if (conn->insert(downloadCount) != 1)
    {
        cout <<endl
            << __FILE__ << " : " << __LINE__ << "  insert error" << endl;
        return SQL_STATUS::EXE_err;
    }
    else
        return SQL_STATUS::EXE_sus;
}


SQL_STATUS BookDownloadCount::set_newest_count(const int &autoBookId, const string &bookId, const int &times, const string &dayTime, const string &bookName)
{
    BookDownloadCountTable downloadCount;
    downloadCount.autoBookId = autoBookId;
    downloadCount.bookId = bookId;
    downloadCount.bookName = bookName;
    downloadCount.dayTime = dayTime;
    downloadCount.times = times;
    return this->set_newest_count(downloadCount);
}


SQL_STATUS BookDownloadCount::get_newest_count_by_id(BookDownloadCountTable &downloadCount, const string &bookId)
{
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "where bookId = " + bookId;
	// auto res = conn->query<BookDownloadCountTable>(state);
	auto res = conn->query<BookDownloadCountTable>();
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    downloadCount = res[0];
	return SQL_STATUS::EXE_sus;
}



#endif


