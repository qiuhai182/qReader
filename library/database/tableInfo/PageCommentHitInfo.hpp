#pragma once

#ifndef _PAGECOMMENTHITINFO_H
#define _PAGECOMMENTHITINFO_H

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

namespace ormpp
{
    /*********
     * 书籍页评论点赞
     * **/
    struct PageCommentHitInfoTable
    {
        int commentId;	    // commentId 外键
        int hitter  ;
        int praised ;
        int page ;          //
        string bookId;	    // bookId
    };
    REFLECTION(PageCommentHitInfoTable, commentId,hitter,praised,page,bookId);

    class PageCommentHitInfo{
    public: 
        PageCommentHitInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create PageCommentHitInfoTable error ";
            }
        }

        SQL_STATUS insert_hit(const PageCommentHitInfoTable & page_comment_hit);
        SQL_STATUS delete_hit_by_commentId_hitter(const int comment_id,const int&hitter); 
        int is_hit_commented(const int & hitter, const int & comment_id);
        SQL_STATUS delete_hit_by_commentId(const int & comment_id);

        
    private:
        SQL_STATUS create_table();
        // SQL_STATUS create_double_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };  
}


SQL_STATUS PageCommentHitInfo::create_table()
{
    /*创建书籍页评分点赞信息表 */
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
    conn->execute("DROP TABLE IF EXISTS PageCommentHitInfoTable");
    string state = 
    "CREATE TABLE PageCommentHitInfoTable( "
        " commentId INTEGER NOT NULL, "
        " hitter INTEGER NOT NULL , "
        " praised INTEGER NOT NULL , "
        " page INTEGER NOT NULL , "
        " bookId TEXT NOT NULL, "
        " PRIMARY KEY (commentId) ,"
        " CONSTRAINT page_comment_hitId FOREIGN KEY (commentId) REFERENCES  PageCommentInfoTable(commentId)"
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS PageCommentHitInfo::insert_hit(const PageCommentHitInfoTable & page_comment_hit)
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

	int ret = conn->insert<PageCommentHitInfoTable>(page_comment_hit);
     if( 1 != ret ){
        cout << __FILE__ << " : " << __LINE__ 
            << "insert PageCommentHitInfoTable  error" << endl;
        return SQL_STATUS::EXE_err;
    }
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentHitInfo::delete_hit_by_commentId_hitter(const int comment_id,const int&hitter)
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
    string state =  
        " delete from PageCommentHitInfoTable  where "
        " commentId = " + to_string(comment_id) + 
        " and  hitter =  " + to_string(hitter) ;
    
	return execute_sql(conn," delete PageCommentHitInfoTable  a hit",state);
}

int PageCommentHitInfo::is_hit_commented(const int & hitter, const int & comment_id)
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

    string state = 
        " select *  from PageCommentHitInfoTable  where "
        " commentId = " + to_string(comment_id) + 
        " and  hitter =  " + to_string(hitter) ;
    auto res = conn->query<PageCommentHitInfoTable>(state);
    if(res.size() ==0 )
        return 0 ;
    else
        return 1 ;
}

SQL_STATUS PageCommentHitInfo::delete_hit_by_commentId(const int & comment_id)
{//删除某一评论全部点赞
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state =  
        " delete from PageCommentHitInfoTable "
        " where commentId = " + to_string(comment_id) ;
	return execute_sql(conn,"delete PageCommentHitInfoTable some hit  ",state);
}


#endif
