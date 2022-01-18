
#pragma once

#ifndef _PAGECOMMENTSQL_H
#define _PAGECOMMENTSQL_H

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
#include "tableInfo/PageCommentInfo.hpp"
#include "tableInfo/PageCommentHitInfo.hpp"
#include "tableInfo/UserInfo.hpp"

using namespace ormpp;
using namespace std;

namespace ormpp
{
    struct pageCommentRes
    {
        string title;
        string content;
        int reviewer  ;     //
        int hitCount; //
        string remarkTime ;
        int replyCount  ;     //
        int isUpdateHead; //
        string nickname; //
        int commentId;
        int isHited;
    };

    struct pageCommentParameter
    {
        string bookId;
        int page;
        int observer;
        int offset;
        int count;
        bool reverse;
        int parentId; //可选
    };

    /** 
     * 某页评论信息操作
     */
    //一个联合查询页面评论的结果结构 
    //标题，内容，点赞数，评论者userId,评论时间,回复数,是否更改头像,昵称,commentId
    typedef tuple<string,string,int,int,string,int,int,string,int>   sqlPageComRes ;

    class PageCommentImpl
    {

    public:
        PageCommentImpl()
        {
            __user = new UserInfo();
            __comment = new PageCommentInfo();
            __comHit = new PageCommentHitInfo();
        }
        ~PageCommentImpl()
        {
            delete __user;
            delete __comment;
            delete __comHit;
        }
        SQL_STATUS get_supper_comment_by_time(vector<pageCommentRes> & res ,const pageCommentParameter& para);
        SQL_STATUS get_supper_comment_by_hit(vector<pageCommentRes> & res ,const pageCommentParameter& para);

        SQL_STATUS get_sub_comment_by_time(vector<pageCommentRes> & res ,const pageCommentParameter& para);
        SQL_STATUS get_sub_comment_by_hit(vector<pageCommentRes> & res ,const pageCommentParameter& para);

        SQL_STATUS hit_comment(const PageCommentHitInfoTable & data);
        SQL_STATUS cancal_hit_comment_by_commentId_hitter(const int & comment_id,const int & hitter);       
        SQL_STATUS add_comment(const PageCommentInfoTable & data);
        SQL_STATUS delete_comment(const int & comment_id ,const int & comment_type);
        int is_exist_supper_comment(const int & comment_id);
        int is_existing(const int & comment_id);
        SQL_STATUS get_max_commentId(int & max_comment_id);
    private:
        SQL_STATUS delete_some_sub_comment(const int &comment_id);
        SQL_STATUS delete_one_sub_comment(const int &comment_id);
        SQL_STATUS delete_supper_comment(const int & comment_id);

        // SQL_STATUS add_supper_comment(const PageCommentInfo & data);
        // SQL_STATUS add_sub_comment(const PageCommentInfo & data);

        void get_full_comment(const int & observer,const vector<sqlPageComRes> & in,vector<pageCommentRes> & res);
    private:
        UserInfo * __user;
        PageCommentHitInfo * __comHit ;
        PageCommentInfo * __comment;
    };

}

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@某页评论点赞信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/

/***********************************public***********************************/

SQL_STATUS PageCommentImpl::delete_comment(const int & comment_id ,const int & comment_type)
{
    if(comment_type < 0 || comment_type > 1)
        return SQL_STATUS::Illegal_info;
    if(comment_type == 0)
        return delete_supper_comment(comment_id);
    else    
        return delete_one_sub_comment(comment_id);
}

SQL_STATUS PageCommentImpl::get_supper_comment_by_time(vector<pageCommentRes> & res ,const pageCommentParameter& para)
{//默认时间升序
    if(para.offset < 0 || para.count < 0)
        return SQL_STATUS::Illegal_info ;
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = para.reverse == true ? "desc":"asc" ;

    string state  = 
        " select C.title ,C.content,C.hitCount,C.reviewer,C.remarkTime, C.replyCount,U.isUpdateHead ,U.userNickName  "
        " from PageCommentInfoTable C  "
        " join UserInfoTable  U "  
        " on U.userId = C.reviewer "
        " where C.bookId = \'" + para.bookId + "\'" +
        " and C.page = " + to_string(para.page) + 
        " order by C.remarkTime  " + order +  
        " limit " + to_string(para.offset) + " , " +  to_string(para.count);
    
    auto buffer = conn->query<sqlPageComRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(para.observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;

}

SQL_STATUS PageCommentImpl::get_supper_comment_by_hit(vector<pageCommentRes> & res,const pageCommentParameter& para)
{//默认评分升序,时间升序
    if(para.offset < 0 || para.count < 0)
        return SQL_STATUS::Illegal_info ;
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = para.reverse == true ? "desc":"asc" ;

    string state  = 
        " select C.title ,C.content,C.hitCount,C.reviewer,C.remarkTime, C.replyCount,U.isUpdateHead ,U.userNickName  "
        " from PageCommentInfoTable C  "
        " join UserInfoTable  U "  
        " on U.userId = C.reviewer "
        " where C.bookId = \'" + para.bookId + "\'" +
        " and C.page = " + to_string(para.page) + 
        " order by C.hitCount  " + order +  
        " , from_unixtime(C.remarkTime) desc"
        " limit " + to_string(para.offset) + " , " +  to_string(para.count);
    
    auto buffer = conn->query<sqlPageComRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(para.observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;

}

SQL_STATUS PageCommentImpl::get_sub_comment_by_time(vector<pageCommentRes> & res ,const pageCommentParameter& para)
{//默认时间升序
    if(para.offset < 0 || para.count < 0)
        return SQL_STATUS::Illegal_info ;
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = para.reverse == true ? "desc":"asc" ;

    string state  = 
        " select C.title ,C.content,C.hitCount,C.reviewer,C.remarkTime, C.replyCount,U.isUpdateHead ,U.userNickName  "
        " from PageCommentInfoTable C  "
        " join UserInfoTable  U "  
        " on U.userId = C.reviewer "
        " where C.bookId = \'" + para.bookId + "\'" +
        " and C.page = " + to_string(para.page) + 
        "and parentId =  " + to_string(para.parentId) +
        " order by C.remarkTime  " + order +  
        " limit " + to_string(para.offset) + " , " +  to_string(para.count);
    
    auto buffer = conn->query<sqlPageComRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(para.observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;
    

}

SQL_STATUS PageCommentImpl::get_sub_comment_by_hit(vector<pageCommentRes> & res ,const pageCommentParameter& para)
{//默认评分升序,时间升序
    if(para.offset < 0 || para.count < 0)
        return SQL_STATUS::Illegal_info ;
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = para.reverse == true ? "desc":"asc" ;

    string state  = 
        " select C.title ,C.content,C.hitCount,C.reviewer,C.remarkTime, C.replyCount,U.isUpdateHead ,U.userNickName  "
        " from PageCommentInfoTable C  "
        " join UserInfoTable  U "  
        " on U.userId = C.reviewer "
        " where C.bookId = \'" + para.bookId + "\'" +
        " and C.page = " + to_string(para.page) + 
        "and parentId =  " + to_string(para.parentId) +
        " order by C.hitCount  " + order +  
        " , from_unixtime(C.remarkTime) desc"
        " limit " + to_string(para.offset) + " , " +  to_string(para.count);
    
    auto buffer = conn->query<sqlPageComRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(para.observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;

}

SQL_STATUS PageCommentImpl::add_comment(const PageCommentInfoTable & data)
{
    return __comment->insert_comment(data);
}

int PageCommentImpl::is_existing(const int & comment_id)
{
    return __comment->is_existing(comment_id);
}

int PageCommentImpl::is_exist_supper_comment(const int & comment_id)
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
    else if(res[0].parentId != 0)
        return 0;//非顶层评论
    else
        return 1;

}

SQL_STATUS PageCommentImpl::get_max_commentId(int & max_comment_id)
{
    return __comment->get_max_commentId(max_comment_id);
}

/***********************************private**********************************/

void PageCommentImpl::get_full_comment(const int & observer,const vector<sqlPageComRes> & in,vector<pageCommentRes> & res)
{
    //标题，内容，点赞数，评论者userId,评论时间,回复数,是否更改头像,昵称,commentId
    //typedef tuple<string,string,int,int,string,int,int,string,int>   sqlPageComRes ;

    for(auto & temp:in)
    {
        pageCommentRes buffer;
        buffer.title =std::move(get<0>(temp));
        buffer.content = std::move(get<1>(temp));
        buffer.hitCount = std::move(get<2>(temp));
        buffer.reviewer = std::move(get<3>(temp));
        buffer.remarkTime = std::move(get<4>(temp));
        buffer.replyCount = std::move(get<5>(temp));
        buffer.isUpdateHead = std::move(get<6>(temp));
        buffer.nickname = std::move(get<7>(temp));
        int ret = __comHit->is_hit_commented(observer,get<8>(temp)) ;
        if( ret != 1)
            continue;
        res.push_back(std::move(buffer));
    }
}

SQL_STATUS PageCommentImpl::delete_one_sub_comment(const int &comment_id)
{//删除普通子评论
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    auto ret = __comHit->delete_hit_by_commentId(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    ret =  __comment->delete_comment(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    //最后执行
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentImpl::delete_some_sub_comment(const int &comment_id)
{//删除大量子评论
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string cond = " where parentId = " + to_string(comment_id);

	auto res = conn->query<PageCommentInfoTable>(cond);
    if(res.size() ==  0)
        return SQL_STATUS::EXE_sus; // 无子评论
    
    for(auto & temp : res)
    {   
        __comHit->delete_hit_by_commentId(temp.commentId);
        __comment->delete_comment(temp.commentId);
    }
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentImpl::delete_supper_comment(const int & comment_id)
{//删除父评论
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    auto ret = delete_some_sub_comment(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    
    ret = __comHit->delete_hit_by_commentId(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    ret =  __comment->delete_comment(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    
    //最后执行
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS PageCommentImpl::hit_comment(const PageCommentHitInfoTable & data)
{
    int ret = __comment->is_existing(data.commentId);
    if(ret != 1)
        return SQL_STATUS::Illegal_info;//可去除
    SQL_STATUS status = __comHit->insert_hit(data);
    if(status != SQL_STATUS::EXE_sus)
        return status;
    return __comment->increase_comment_hit(data.commentId);
}

SQL_STATUS PageCommentImpl::cancal_hit_comment_by_commentId_hitter(const int & comment_id,const int & hitter)
{
    int ret = __comment->is_existing(comment_id);
    if(ret != 1)
        return SQL_STATUS::Illegal_info;
     SQL_STATUS status = __comHit->delete_hit_by_commentId_hitter(comment_id,hitter);
    if(status != SQL_STATUS::EXE_sus)
        return status;
    return __comment->decrease_comment_hit(comment_id);
}


#endif


