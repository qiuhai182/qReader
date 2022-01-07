#pragma once

#ifndef _BOOKCOMMENTHITINFO_H
#define _BOOKCOMMENTHITINFO_H

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
     * 书籍评论信息查询
     */
    struct BookCommentHitInfoTable
    {
        int autoBookId ;        // 外键 
        string bookId ;         // bookId
        int hitter;             // 
        int praised ;           // 
    };
    REFLECTION(BookCommentHitInfoTable, autoBookId,bookId,hitter, praised);
    

    class BookCommentHitInfo{
    public: 
        BookCommentHitInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create BookCommentHitInfo error ";
            }
        }

        SQL_STATUS get_hit_count_by_bookId_praised(const string & book_id,const int & praised,int & hit_count);
        int is_hit_commented(const int & hitter,const string & book_id,const int & praised);
        SQL_STATUS insert_hit(const BookCommentHitInfoTable & data);
        SQL_STATUS insert_hit(const int & auto_book_id,const string & book_id,const int & hitter,const int & praised);
        SQL_STATUS delete_hit_by_bookId_praised(const int & hitter,const string & book_id,const int & praised);
        SQL_STATUS delete_hit_by_comment_by_bookId_praised(const string & book_id,const int & praised);

        /*占位 将实现*/
        // virtual SQL_STATUS del_book();
        // virtual SQL_STATUS up_book();
        // virtual SQL_STATUS up_book();

    private:
        SQL_STATUS create_table();
        // SQL_STATUS create_double_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };

}



/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@评论点赞信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/

SQL_STATUS BookCommentHitInfo::create_table()
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
    conn->execute("DROP TABLE IF EXISTS BookCommentHitInfoTable");
    string state = 
    "CREATE TABLE BookCommentHitInfoTable( "
        " autoBookId INTEGER NOT NULL, "
        " bookId TEXT NOT NULL, "
        " hitter INTEGER NOT NULL , "
        " praised INTEGER NOT NULL , "
        " PRIMARY KEY (autoBookId,hitter,praised) ,"
        " CONSTRAINT grade_stat_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) , "
        " CONSTRAINT hitter_id FOREIGN KEY (hitter) REFERENCES  UserInfoTable(userId) , "
        " CONSTRAINT parised_id FOREIGN KEY (praised) REFERENCES  UserInfoTable(userId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS BookCommentHitInfo::get_hit_count_by_bookId_praised(const string & book_id,const int & praised,int & hit_count)
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

    string state = "select count(autoBookId) from BookCommentHitInfoTable "
                    " group by  praised , bookId having " 
                    " praised = "  + to_string(praised ) + 
                    " and bookId = \'" + book_id + "\'"  ;
	auto res = conn->query<std::tuple<int>>(state);
    if(res.size() == 0){
        hit_count = 0;
    }   
    else
    {
        hit_count = (int)std::get<0>(res[0]) ;
    }
    
	return SQL_STATUS::EXE_sus;
}

int BookCommentHitInfo::is_hit_commented(const int & hitter,const string & book_id,const int & praised)
{//是否点赞 -1: eroor 0:false 1:true 

    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return -1;
    }
    string cond  = " where hitter = " + to_string(hitter) + 
                    " and praised = " + to_string(praised) + 
                    " and bookId = \'" + book_id + "\'" ;
    cout<<" cond is "<<cond <<endl;
    auto res = conn->query<BookCommentHitInfoTable>(cond);
    if(res.size() == 0 )
        return 0 ;
    else   
        return 1;
}

SQL_STATUS BookCommentHitInfo::insert_hit(const BookCommentHitInfoTable & data)
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
    if (conn->insert(data) != 1)
    {
        cout << __FILE__ << " : " << __LINE__ 
            << "insert BookCommentHitInfoTable  error" << endl;
        return SQL_STATUS::EXE_err;
    }
    else
        return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookCommentHitInfo::insert_hit(const int & auto_book_id,const string & book_id,const int & hitter,const int & praised)
{
    BookCommentHitInfoTable buffer;
    buffer.autoBookId = auto_book_id;
    buffer.bookId = book_id;
    buffer.hitter = hitter ;
    buffer.praised = praised ;

    return insert_hit(buffer);
}

SQL_STATUS BookCommentHitInfo::delete_hit_by_bookId_praised(const int & hitter,const string & book_id,const int & praised)
{//删除点赞
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string state =" delete from BookCommentHitInfoTable where " 
                  " hitter = " + to_string(hitter) + 
                  " and  praised = " + to_string(praised) + 
                  " and bookId = \'" + book_id + "\'";
    return execute_sql(conn,"delete BookCommentHitInfoTable a hit ",state);      
}

SQL_STATUS BookCommentHitInfo::delete_hit_by_comment_by_bookId_praised(const string & book_id,const int & praised)
{//删除评论
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state =" delete from BookCommentHitInfoTable where " 
                  " praised = " + to_string(praised) + 
                  " and bookId = \'" + book_id + "\'";
    return execute_sql(conn,"delete BookCommentHitInfoTable some hit",state);      
}

#endif
