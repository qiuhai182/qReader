#pragma once

#ifndef _BOOKSHELFINFO_H
#define _BOOKSHELFINFO_H

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
     * 书架信息查询
     */
    struct UserShelfTable
	{ // 用户个人书架信息表结构
		int userId;			    // 用户id
        int autoBookId;	   	    //
		string bookId;			// 书籍id
		int    isRemove;	   	// 是否移除
        string addTime ;        // 添加时间
	};

    REFLECTION(UserShelfTable,userId,autoBookId,bookId, isRemove,addTime);

    class UserShelfInfo
    {
    public: 
        UserShelfInfo(const bool create_status = true){
            __isCreate = create_status;
			if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create UserShelfTable error ";
			}
        }
        SQL_STATUS get_book_by_userid(vector<UserShelfTable> &books, const int  &user_id);
        SQL_STATUS get_book_by_userid_bookid( UserShelfTable& book, const string &book_id, const int  &user_id);
        SQL_STATUS repeat_add_book(const UserShelfTable& book);
        SQL_STATUS remove_shelf_book(const int &user_id, const string &book_id);
        SQL_STATUS insert_shelf(const UserShelfTable &book);
        SQL_STATUS insert_shelf(const int &user_id, const string &book_id,
                                        const int & auto_book_id,const string &add_time);

        // SQL_STATUS up_book();
        // SQL_STATUS up_book();
		
    private:
		SQL_STATUS create_table();
        // SQL_STATUS create_times_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };
}

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书架书籍信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS UserShelfInfo::create_table()
{
    /*创建书籍搜索信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;

    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    conn->execute("DROP TABLE IF EXISTS UserShelfTable");
    string state = 
    "CREATE TABLE UserShelfTable( "
        " userId INTEGER NOT NULL, "
        " autoBookId INTEGER NOT NULL,"
        " bookId TEXT NOT NULL , "
        " isRemove INTEGER NOT NULL DEFAULT 0 , "
        " addTime TEXT NOT NULL , "
        " PRIMARY KEY (userId,autoBookId) ,"
        " CONSTRAINT shelf_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS UserShelfInfo::get_book_by_userid(vector<UserShelfTable> &books, const int &user_id)
{// 通过userid获取书架
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = " where userId = " + to_string(user_id) + 
                "  and isRemove = 0 ";
	auto res = conn->query<UserShelfTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	else
		books = std::move(res);
	return SQL_STATUS::EXE_sus;


}

SQL_STATUS UserShelfInfo::get_book_by_userid_bookid( UserShelfTable& book, const string &book_id, const int  &user_id)
{// 通过userId,bookId获取书架
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = " where userId = " + to_string(user_id) + 
                " and bookId = \'" + book_id + "\'" ;
	auto res = conn->query<UserShelfTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	else
		book = std::move(res[0]);
	return SQL_STATUS::EXE_sus;

}

SQL_STATUS UserShelfInfo::repeat_add_book(const UserShelfTable& book)
{//在移除后的添加 若本来无此书将返回 EXE_err
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

    UserShelfTable buffer ;
    SQL_STATUS ret =  get_book_by_userid_bookid(buffer,book.bookId,book.userId);
    if(ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;
    //已经移除过 更新添加
    string state = string("update  UserShelfTable set ") + 
                " isRemove  = 0  , addTime = \'" + book.addTime + "\'"+
                " where  userId = " + to_string(book.userId) + 
                " and bookId = \'" + book.bookId + "\'" ;

	return execute_sql(conn, "repeat inset UserShelfTable",state);

}

SQL_STATUS UserShelfInfo::remove_shelf_book(const int &user_id, const string &book_id)
{// 数据库删除用户书架里书籍
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

	string state = "update  UserShelfTable set " 
                " isRemove  = 1  where  " 
                " userId = " + to_string(user_id) + 
                " and bookId = \'" + book_id + "\'" ;
    return execute_sql(conn ,"remove  book from UserShelfTable",state);
}

SQL_STATUS UserShelfInfo::insert_shelf(const UserShelfTable &book)
{//插入书籍
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
    //重新添加判定
    SQL_STATUS ret = repeat_add_book(book);
    if(ret == SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_sus;

    if (conn->insert(book) != 1)
    {
        cout << __FILE__ << " : " 
            << __LINE__ 
            << "insert UserShelfTable error" << endl;
        return SQL_STATUS::EXE_err;
    }
    else
        return SQL_STATUS::EXE_sus;

}

SQL_STATUS UserShelfInfo::insert_shelf(const int &user_id, const string &book_id,
                                        const int & auto_book_id,const string &add_time)
{
    UserShelfTable buffer;
    buffer.addTime = add_time;
    buffer.userId = user_id;
    buffer.bookId = book_id;
    buffer.isRemove = 0 ;
    buffer.addTime = auto_book_id;
    return insert_shelf(buffer);
}


#endif
