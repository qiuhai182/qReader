#pragma once

#ifndef _BOOKSEARCHINFO_H
#define _BOOKSEARCHINFO_H

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
     * 书籍搜索次数信息查询
     */
    struct SearchStatisticsTable
	{ // 搜索书籍次数统计
		int autoBookId ;        // 外键
        string bookId;          // 书籍Id
		string dayTime;			// 数据记录日期，精确到天 
		int times; 				// 搜索次数
		string bookName;		// 书籍name
	};
    REFLECTION(SearchStatisticsTable,autoBookId, bookId, dayTime,times,bookName);

    class BookSearchInfo{
    public: 
        BookSearchInfo(const bool create_status = true){
            __isCreate = create_status;
			if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create SearchStatisticsTable error ";
			}
        }
        SQL_STATUS get_mostly_search_by_month_count(const string & monthTime,
                                vector<SearchStatisticsTable> &searchList,const int & count);
        SQL_STATUS get_searchStatistics_info_by_id_and_daytime(const string &bookId, 
                                const string &dayTime,SearchStatisticsTable & stat);
        SQL_STATUS insert_seacrh_inf(const SearchStatisticsTable & stat);
        SQL_STATUS update_seacrh_inf(const SearchStatisticsTable & stat);
        SQL_STATUS plus_search_times(const int & auto_book_id,const string &book_id,
                            const string & dayTime,const string & bookName);
        /*占位 将实现*/
        // virtual SQL_STATUS del_book();
        // virtual SQL_STATUS up_book();
        // virtual SQL_STATUS up_book();
		
    private:
		SQL_STATUS create_table();
        SQL_STATUS create_times_index();
        SQL_STATUS create_indexs();
        bool __isCreate;
    };
}

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍搜索信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS BookSearchInfo::create_table()
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
    conn->execute("DROP TABLE IF EXISTS SearchStatisticsTable");
    string state = 
    "CREATE TABLE SearchStatisticsTable( "
        " autoBookId INTEGER NOT NULL, "
        " bookId text NOT NULL , "
        " dayTime text NOT NULL , "
        " times INTEGER NOT NULL DEFAULT 0 , "
        " bookName text  NOT NULL, "  
        " PRIMARY KEY (autoBookId) ,"
        " CONSTRAINT search_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB3 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS BookSearchInfo::create_times_index()
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
    string state; 
    state = "ALTER TABLE SearchStatisticsTable ADD INDEX times_index (times)" ;
    return execute_sql(conn,"create SearchStatisticsTable  times_index ",state);
}
    
SQL_STATUS BookSearchInfo::create_indexs()
{
    return create_times_index();
}

SQL_STATUS BookSearchInfo::get_mostly_search_by_month_count(const string & monthTime,
                                vector<SearchStatisticsTable> &searchList,const int & count)
{ // 分页查找
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "  conn is NULL  "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond 
		=  "where dayTime LIKE \"\%" + monthTime + "\%\" order by  times  desc limit " + std::to_string(count); ;
	auto res = conn->query<SearchStatisticsTable>(cond);
	if (res.size() == 0)
	{
		cout << "FILE: " << __FILE__ 
			 << "  mostly search error "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::EXE_err;
	}
	else
	{
		searchList = std::move(res);
		return SQL_STATUS::EXE_sus;
	}
}

SQL_STATUS BookSearchInfo::get_searchStatistics_info_by_id_and_daytime(const string &bookId, 
                        const string &dayTime,SearchStatisticsTable & stat)
{//通过bookid,daytime获得次数
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "  conn is NULL  "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond 
		= " where bookId = \'" + bookId + "\' and  dayTime = \'" + dayTime + "\'";
	auto res = conn->query<SearchStatisticsTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	else
		stat = std::move(res[0]);
	return SQL_STATUS::EXE_sus;

}

SQL_STATUS BookSearchInfo::insert_seacrh_inf(const SearchStatisticsTable & stat)
{//插入新的搜索信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if(conn == NULL)
	{
		cout << " FILE: " << __FILE__ 
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	if(conn->insert(stat) != 1)
	{
		cout<<__FILE__ << " : " << __LINE__ 
            << " SearchStatistics insert error "<<endl ;
		return SQL_STATUS::EXE_err ;
	}
	return SQL_STATUS::EXE_sus;

}

SQL_STATUS BookSearchInfo::update_seacrh_inf(const SearchStatisticsTable & stat)
{//更新书本当天的搜索信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if(conn == NULL)
	{
		cout<< "FILE: " << __FILE__ 
			 << "  conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = "update SearchStatisticsTable set  times = " 
			+ to_string(stat.times) + "  where bookId = \'" + stat.bookId 
			+ "\'  and  dayTime  = \'" + stat.dayTime + "\'" ;
	cout<<" cond s "<<cond<<endl;
	if(conn->execute(cond) == INT_MIN)
	{
		cout<<__FILE__ << " : " << __LINE__ 
            << " SearchStatistics update error "<<endl ;
		return SQL_STATUS::EXE_err ;
	}
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookSearchInfo::plus_search_times(const int & auto_book_id,const string &book_id,
                            const string & dayTime,const string & bookName)
{ // 搜索时更新搜索时间
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout<< "FILE: " << __FILE__ 
			 << "  conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	SearchStatisticsTable stat{auto_book_id,book_id, dayTime,1,bookName}; ;
	if( get_searchStatistics_info_by_id_and_daytime(book_id, dayTime,stat) == SQL_STATUS::EXE_err)
	{//一天该书新搜索
		if(insert_seacrh_inf(stat) == SQL_STATUS::EXE_sus){
			return SQL_STATUS::EXE_sus ;
		}else{
			return SQL_STATUS::EXE_err;
		}
	}
	else//搜索加一
	{ 
		stat.times += 1;
		if(update_seacrh_inf(stat) == SQL_STATUS::EXE_sus){
			return SQL_STATUS::EXE_sus ;
		}else{
			return SQL_STATUS::EXE_err;
		}
	}

}




#endif
