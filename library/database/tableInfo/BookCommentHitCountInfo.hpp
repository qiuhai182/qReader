#pragma once

#ifndef _BOOKCOMMENTHICOUNTINFO_H
#define _BOOKCOMMENTHICOUNTINFO_H

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
     * 书籍评论点赞统计信息查询
     */
    struct BookCommentHitCountInfoTable
    {
        int autoBookId ;        // 外键 
        string bookId ;         // bookId 
        int praised ;           // 被赞评论userId
        int count;              // 被点赞数
    };
    REFLECTION(BookCommentHitCountInfoTable, autoBookId,bookId,praised,count);
    

    class BookCommentHitCountInfo{
    public: 
        BookCommentHitCountInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create BookCommentHitCountInfo error ";
            }
        }
        //对外提供
        SQL_STATUS plus_hit_count_by_bookId_praised(const string & book_id,const int & praised);
        SQL_STATUS sub_hit_count_by_bookId_praised(const string & book_id,const int & praised);
        SQL_STATUS get_hit_count_by_bookId_praised(const string & book_id,const int & praised,int & hit_count);
        int is_existing(const string & book_id,const int & praised);
        SQL_STATUS insert_hit(const BookCommentHitCountInfoTable & data);
        SQL_STATUS insert_hit(const int & auto_book_id,const string & book_id,const int & praised,const int &count); 
        SQL_STATUS delete_hit_by_bookId_praised(const string & book_id,const int & praised);

        /*占位 将实现*/
        // virtual SQL_STATUS del_book();
        // virtual SQL_STATUS up_book();
        // virtual SQL_STATUS up_book();

    private:
        SQL_STATUS create_table();
        SQL_STATUS update_hit_count_by_bookId_praised(const BookCommentHitCountInfoTable & data);
        // SQL_STATUS create_double_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };



/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@评论点赞统计信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS BookCommentHitCountInfo::create_table()
{
    /*创建书籍评分统计信息表 */
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
        struct BookCommentHitCountInfoTable
    {
        int autoBookId ;        // 外键 
        string bookId ;         // bookId 
        int praised ;           // 被赞评论userId
        int count;              // 被点赞数
    };
    conn->execute("DROP TABLE IF EXISTS BookCommentHitCountInfoTable");
    string state = 
    "CREATE TABLE BookCommentHitCountInfoTable( "
        " autoBookId INTEGER NOT NULL, "
        " bookId TEXT NOT NULL, "
        " praised INTEGER NOT NULL , "
        " count INTEGER NOT NULL ,"
        " PRIMARY KEY (autoBookId,praised) ,"
        " CONSTRAINT hit_stat_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) , "
        " CONSTRAINT hit_parised_id FOREIGN KEY (praised) REFERENCES  UserInfoTable(userId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS BookCommentHitCountInfo::plus_hit_count_by_bookId_praised(const string & book_id,const int & praised)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string cond = "where bookId = \'" + book_id + "\'  and praised = "  +to_string(praised) ;
	auto res = conn->query<BookCommentHitCountInfoTable>(cond);
    
    
    if(res.size() == 0){
        return SQL_STATUS::Illegal_info;
    }   
    else
    {
        res[0].count += 1 ;
        return update_hit_count_by_bookId_praised(res[0]);
    }
    
	return SQL_STATUS::EXE_sus;
    
}

SQL_STATUS BookCommentHitCountInfo::sub_hit_count_by_bookId_praised(const string & book_id,const int & praised)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string cond = "where  bookId = \'" + book_id + "\'  and praised = "  +to_string(praised) ;
	auto res = conn->query<BookCommentHitCountInfoTable>(cond);
    
    
    if(res.size() == 0){
        return SQL_STATUS::Illegal_info;
    }   
    else
    {   if(res[0].count >0 )
        {
            res[0].count -= 1 ;
            return update_hit_count_by_bookId_praised(res[0]);
        }
        return SQL_STATUS::Illegal_info;
        
    }
}

SQL_STATUS BookCommentHitCountInfo::get_hit_count_by_bookId_praised(const string & book_id,const int & praised,int & hit_count)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string cond = " where bookId = \'" + book_id + "\'  and praised = "  +to_string(praised) ;
	auto res = conn->query<BookCommentHitCountInfoTable>(cond);
    
    
    if(res.size() == 0){
        hit_count = 0 ;
    }   
    else
    {
        hit_count = res[0].count ;
    }
    return SQL_STATUS::EXE_sus;
}

int BookCommentHitCountInfo::is_existing(const string & book_id,const int & praised)
{//在该表中是否存在 -1:池错误 0:false 1:true
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return -1;
    }

    string cond = "where  bookId = \'" + book_id + "\'  and praised = "  +to_string(praised) ;
	auto res = conn->query<BookCommentHitCountInfoTable>(cond);
    
    if(res.size() == 0){
        return 0;
    }   
    else
    {
       return 1;
    }
}

SQL_STATUS BookCommentHitCountInfo::insert_hit(const BookCommentHitCountInfoTable & data)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    int ret = conn->insert<BookCommentHitCountInfoTable>(data);
    if( 1 != ret ){
        cout << __FILE__ << " : " << __LINE__ 
            << "insert BookCommentHitCountInfoTable  error" << endl;
        return SQL_STATUS::EXE_err;
    }
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookCommentHitCountInfo::insert_hit(const int & auto_book_id,const string & book_id,const int & praised,const int &count)
{
    BookCommentHitCountInfoTable buffer;
    buffer.autoBookId = auto_book_id;
    buffer.bookId = book_id;
    buffer.praised = praised ;
    buffer.count = count;
    return insert_hit(buffer);
}

SQL_STATUS BookCommentHitCountInfo::update_hit_count_by_bookId_praised(const BookCommentHitCountInfoTable & data)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

	int ret = conn->update<BookCommentHitCountInfoTable>(data);
    
    
    if( ret != 1){
        return SQL_STATUS::Illegal_info;
    }   
    else
    {
        return SQL_STATUS::EXE_sus;
    }
}

SQL_STATUS BookCommentHitCountInfo::delete_hit_by_bookId_praised(const string & book_id,const int & praised)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string state =" delete from BookCommentHitCountInfoTable where " 
                  " praised = " + to_string(praised) + 
                  " and bookId = \'" + book_id + "\'";
    return execute_sql(conn,"delete BookCommentHitCountInfoTable comment",state);      

}




}




#endif
