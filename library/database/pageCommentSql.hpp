
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

    /** 
     * 某页评论信息操作
     */
    //一个联合查询评论的结果结构 
    //标题、具体内容、评分、userid、昵称、是否更改头像,时间 ,bookId  不包含是否已经点赞,被点赞数
    typedef tuple<string,string,int,int,string,int,string,string,string>  sqlComHitRes;
    //返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
    //为点赞数评论查找的结果  标题、具体内容、评分、昵称,是否、更改头像时间
    typedef tuple<string,string,int,string,int,string>  inforCount;
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
        SQL_STATUS get_supper_comment_by_time(vector<commentRes> & res ,const string & book_id ,
                                                    const int & page ,const bool & reverse = false);
        SQL_STATUS get_supper_comment_by_hit(vector<commentRes> & res ,const string & book_id ,
                                                    const int & page ,const bool & reverse = false);

        SQL_STATUS get_sub_comment_by_time(vector<commentRes> & res ,
                                                    const int &supper_comment_id,const bool & reverse = false);
         SQL_STATUS get_sub_comment_by_hit(vector<commentRes> & res ,
                                                    const int &supper_comment_id,const bool & reverse = false);
        SQL_STATUS hit_comment(const PageCommentHitInfoTable & data);
        SQL_STATUS cancal_hit_comment_by_commentId_hitter(const int & comment_id,const int & hitter);       
        SQL_STATUS add_comment(const PageCommentInfo & data);
        SQL_STATUS delete_comment(const int & comment_id ,const int & comment_type);
    private:
        string get_cond_by_pattern_reverse(const int &  pattern,const bool reverse);
        SQL_STATUS delete_some_sub_comment(const int &comment_id);
        SQL_STATUS delete_one_sub_comment(const int &comment_id);
        SQL_STATUS delete_supper_comment(const int & comment_id);

        SQL_STATUS get_other_info_for_count(const int & praised,inforCount & res);
        void get_full_comment(const int & observer,const vector<sqlComHitRes> & in,vector<commentRes> & res);
        void get_full_comment(const int & observer,const vector<PageCommentInfoTable> & in,vector<commentRes> & res);
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
    if(comment_id = 0)
        return delete_supper_comment(comment_id);
    else    
        return delete_one_sub_comment(comment_id);
}

/***********************************private**********************************/
string PageCommentImpl::get_cond_by_pattern_reverse(const int &  pattern,const bool reverse)
{// 0:time  1:hit  2:score
    if( pattern < 0 || pattern > 2) 
        return "";
    
    if(pattern == 0)
    {
        return string(" order by from_unixtime(G.remarkTime) ");
    }
    else if(pattern == 1)
    {
        return string(" ");
    }
    else//2
    {
        return string(" ");
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

    ret = __comHit->delete_hit_by_commentId(comment_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    ret =  __comm->delete_comment(comment_id);
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

	auto res = conn->query<PageCommentInfoTable>()
    if(res.size() ==  0)
        return SQL_STATUS::Illegal_info；
    
    for(auto & temp : res)
    {
        __comHit->delete_hit_by_commentId(comment_id);
        __comm->delete_comment(comment_id);
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
    ret =  __comm->delete_comment(comment_id);
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


