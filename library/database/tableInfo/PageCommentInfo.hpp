#pragma once

#ifndef _PAGECOMMENTINFO_H
#define _PAGECOMMENTINFO_H

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
     * 书籍页评论
     * **/
    struct PageCommentInfoTable
    {
        int commentId ;     // 评论id
        int parentId  ;     // 父评论Id
        int reviewer  ;     // 评论者Id 外键
        string title  ;     // 
        string content;     // 
        string remarkTime ; // 评论时间
        int hitCount  ;     // 点赞数
        string bookId ;
        int page ; //
        int replyCount; // 回复人数
    };
    REFLECTION(PageCommentInfoTable, commentId,parentId,reviewer,title, 
                        content,remarkTime,hitCount,bookId,page,replyCount);

    class PageCommentInfo{
    public: 
        PageCommentInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create PageCommentInfoTable error ";
            }
        }

        SQL_STATUS insert_comment(const PageCommentInfoTable & comment);
        SQL_STATUS delete_comment(const int & comment_id);
        SQL_STATUS get_max_commentId(int & max_comment_id);
        SQL_STATUS increase_comment_hit(const int & comment_id);
        SQL_STATUS decrease_comment_hit(const int & comment_id);
        SQL_STATUS increase_comment_reply(const int & comment_id);
        SQL_STATUS decrease_comment_reply(const int & comment_id);

        int is_existing(const int & comment_id);
    private:
        SQL_STATUS create_table();
        // SQL_STATUS create_double_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };  
}


SQL_STATUS PageCommentInfo::create_table()
{
    /*创建书籍页评论信息表 */
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
    conn->execute("DROP TABLE IF EXISTS PageCommentInfoTable");
    string state = 
    "CREATE TABLE PageCommentInfoTable( "
        " commentId INTEGER NOT NULL, "
        " parentId INTEGER NOT NULL, "
        " reviewer INTEGER NOT NULL , "
        " title TEXT NOT NULL , "
        " content TEXT NOT NULL, "
        " remarkTime TEXT NOT NULL, "
        " hitCount INTEGER NOT NULL , "
        " bookId TEXT NOT NULL , "
        " page INTEGER NOT NULL, "
        " replyCount INTEGER NOT NULL ,"
        " PRIMARY KEY (commentId,reviewer,page), "
        " CONSTRAINT page_reviewer_book_id FOREIGN KEY (reviewer) REFERENCES  UserInfoTable(userId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS PageCommentInfo::insert_comment(const PageCommentInfoTable & comment)
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
    // cout<<"ee"
    //     <<comment.commentId
    //     <<" "<<comment.bookId
    //     <<" "<<comment.content
    //     <<" "<<comment.hitCount
    //     <<" "<<comment.page
    //     <<" "<<comment.parentId
    //     <<" "<<comment.replyCount
    //     <<" "<<comment.remarkTime
    //     <<" "<<comment.reviewer
    //     <<endl;
	int ret = conn->insert<PageCommentInfoTable>(comment);
    if( 1 != ret ){
        cout << __FILE__ << " : " << __LINE__ 
             << "    insert PageCommentInfoTable  error  " << endl;
        return SQL_STATUS::EXE_err;
    }
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentInfo::delete_comment(const int & comment_id)
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
            " delete from PageCommentInfoTable  where "
            " commentId = " + to_string(comment_id) ;
     
    return execute_sql(conn," delete PageCommentInfoTable  a comment ",state);
}

SQL_STATUS PageCommentInfo::get_max_commentId(int & max_comment_id)
{//获取可分配最大commentId
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string state = "select max(commentId) from PageCommentInfoTable ";
    auto res = conn->query<tuple<int>>(state);
    if(res.size() ==0 ){
        cout << "FILE: " << __FILE__ << " "
            << " Possible maximum Commentid failure "
            << " LINE  " << __LINE__ << endl;
        max_comment_id = 1;
        return SQL_STATUS::EXE_err;
    }
    max_comment_id = get<0>(res[0]) + 1;
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentInfo::increase_comment_hit(const int & comment_id)
{//点赞加一
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state = " update PageCommentInfoTable set hitCount = hitCount + 1 "
                    "where commentId = " + to_string(comment_id);

    return execute_sql(conn,"increase page comment hit ",state);
}

SQL_STATUS PageCommentInfo::decrease_comment_hit(const int & comment_id)
{//点赞减一
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state = " update PageCommentInfoTable set hitCount = hitCount - 1 "
                    "where commentId = " + to_string(comment_id);

    return execute_sql(conn,"increase page comment hit ",state);

}

SQL_STATUS PageCommentInfo::increase_comment_reply(const int & comment_id)
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
    string state = " update PageCommentInfoTable set replyCount = replyCount + 1 "
                    "where commentId = " + to_string(comment_id);

    return execute_sql(conn,"increase page comment replyCount ",state);
}
SQL_STATUS PageCommentInfo::decrease_comment_reply(const int & comment_id)
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
    string state = " update PageCommentInfoTable set replyCount = replyCount - 1 "
                    "where commentId = " + to_string(comment_id);

    return execute_sql(conn,"decrease page comment replyCount ",state);
}

int PageCommentInfo::is_existing(const int & comment_id)
{//是否存在
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return -1;
    }
    string cond = "where commentId = " + to_string(comment_id);

    auto res = conn->query<PageCommentInfoTable>(cond) ;

    if (res.size() == 0)
        return 0;
    else 
        return 1;

}



#endif
