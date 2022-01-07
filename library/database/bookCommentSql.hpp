#pragma once

#ifndef _BOOKCOMMENTSQL_H
#define _BOOKCOMMENTSQL_H

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
#include "tableInfo/BookCommentHitInfo.hpp"
#include "tableInfo/UserInfo.hpp"
#include "tableInfo/BookCommentHitCountInfo.hpp"
#include "tableInfo/BookBaseInfo.hpp"
#include "tableInfo/BookGradeInfo.hpp"

using namespace ormpp;
using namespace std;

namespace ormpp
{

    /** 
     * 书籍评论信息操作
     */
    //一个联合查询评论的结果结构 
    //标题、具体内容、评分、userid、昵称、是否更改头像,时间 ,bookId  不包含是否已经点赞,被点赞数
    typedef tuple<string,string,int,int,string,int,string,string,string>  sqlComHitRes;
    //返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
    //为点赞数评论查找的结果  标题、具体内容、评分、昵称,是否、更改头像时间
    typedef tuple<string,string,int,string,int,string>  inforCount;
    class BookCommentImpl
    {

    public:
        BookCommentImpl()
        {
            __comHit = new BookCommentHitInfo();
            __user = new UserInfo();
            __hitCount = new BookCommentHitCountInfo();
            __book = new BookBaseInfo();
            __grade = new BookGradeInfo();
        }
        ~BookCommentImpl()
        {
            delete __comHit;
            delete __user;
            delete __hitCount;
            delete __book;
            delete __grade ;
        }
    
        SQL_STATUS get_comment_by_score(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse = false);
        SQL_STATUS get_comment_by_time(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse = false);
        SQL_STATUS get_comment_by_hit(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse = false);
        SQL_STATUS hit_comment_by_bookId_praised(const int & hitter,const string & book_id,const int & praised);
        SQL_STATUS cancal_hit_comment_by_bookId_praised(const int & hitter,const string & book_id,const int & praised);               
        SQL_STATUS add_comment(const string &book_id ,const int &user_id,
                                        const int &book_score,const string & title,
                                        const string &content,const string & remark_time );
        SQL_STATUS delete_comment(const string &book_id ,const int &user_id);
    private:
        SQL_STATUS create_table();
        SQL_STATUS get_other_info_for_count(const int & praised,inforCount & res);
        void get_full_comment(const int & observer,const vector<sqlComHitRes> & in,vector<commentRes> & res);
        void get_full_comment(const int & observer,const vector<BookCommentHitCountInfoTable> & in,vector<commentRes> & res);
        // SQL_STATUS create_double_index();
        // SQL_STATUS create_indexs();
        BookCommentHitInfo * __comHit ;
        UserInfo * __user;
        BookCommentHitCountInfo * __hitCount;
        BookBaseInfo  *__book;
        BookGradeInfo * __grade ;
    };



/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍评论点赞信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
void BookCommentImpl::get_full_comment(const int & observer,const vector<sqlComHitRes> & in,vector<commentRes> & res)
{//添加查看用户是否点赞，被点赞者点赞数
    //一个联合查询评论的结果结构 
    //标题、具体内容、评分、userid、昵称、是否更改头像,时间 ,bookId  不包含是否已经点赞,被点赞数
    //typedef tuple<string,string,int,int,string,int,string,string,string>  sqlComHitRes;
    //返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    //typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
    int size = in.size();
    
    int count_buffer ;
    int  isHit;
    commentRes buffer ;
    for(int index = 0 ; index < size ; index++)
    {  
        get<0>(buffer) = std::move(get<0>(in[index]) );      // 标题 0 -> 0
        get<1>(buffer) = std::move(get<1>(in[index]) );       // 内容 1 -> 1
        get<2>(buffer) = std::move(get<2>(in[index]) );       // 评分 2 -> 2
        get<3>(buffer) = std::move(get<3>(in[index]) );       // userid 3 -> 3
        get<4>(buffer) = std::move(get<4>(in[index]) );       // 昵称 4 -> 4
        get<5>(buffer) = std::move(get<5>(in[index]) );      // 时间 5 -> 5
        get<6>(buffer) = std::move(get<6>(in[index]) );       // 是否更改头像 6 -> 6
        get<7>(buffer) = std::move(get<7>(in[index]) );      // bookId 7 -> 7
        isHit = __comHit->is_hit_commented(observer, get<7>(in[index]),get<3>(in[index]) );
        if( -1 ==  isHit)
            continue;//错误
        get<8>(buffer) = isHit == 1 ?true:false;          // 是否点赞 7,3 -> 7
        //获取点赞数
        if( SQL_STATUS::EXE_sus !=  __hitCount->get_hit_count_by_bookId_praised(get<7>(in[index]), get<3>(in[index]),count_buffer))
            continue;//错误
        get<9>(buffer) = count_buffer;

        res.push_back(buffer);
    }
}

void BookCommentImpl::get_full_comment(const int & observer,
                                    const vector<BookCommentHitCountInfoTable> & in,vector<commentRes> & res)
{//重载 获取bookId,praised,count以外的信息

    //返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    //typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
    //为点赞数评论查找的结果  标题、具体内容、评分、昵称,是否、更改头像、时间
    //typedef tuple<string,string,int,string,int,string>  inforCount;
    int size = in.size();
    inforCount get_info ;
    commentRes res_buffer;
    SQL_STATUS ret ;
    int isHit  = -1;
    for(int index = 0 ;index < size ;index++)
    {
        ret = get_other_info_for_count(in[index].praised,get_info);
        if(ret != SQL_STATUS::EXE_sus)
            continue;//错误
        get<0>(res_buffer) = get<0>(get_info);  // 标题
        get<1>(res_buffer) = get<1>(get_info);  // 内容
        get<2>(res_buffer) = get<2>(get_info);  // 评分
        get<3>(res_buffer) = in[index].praised; // userId
        get<4>(res_buffer) = get<3>(get_info);  // 昵称
        get<5>(res_buffer) = get<4>(get_info);  // 是否更改头像
        get<6>(res_buffer) = get<5>(get_info);  // 时间
        get<7>(res_buffer) = in[index].bookId;  // bookId
        isHit = __comHit->is_hit_commented(observer,in[index].bookId,in[index].praised);
        if(isHit == -1)
            continue;//错误  
        get<8>(res_buffer) = isHit == 1 ?true:false; // 是否点赞
        get<9>(res_buffer) = in[index].count ;  // 点赞数

        res.push_back(res_buffer);
        
    }
}

SQL_STATUS BookCommentImpl::get_comment_by_score(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse )
{//默认评分升序,时间升序
    if(offset < 0 || count < 0)
        return SQL_STATUS::Illegal_info ;
    // 标题、具体内容、评分、userId、昵称、时间
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = reverse == true ? "desc":"asc" ;

    string state  = 
        " select  G.title , G.content , G.bookScore , U.userId , U.userNickName , U.isUpdateHead, from_unixtime(G.remarkTime), G.bookId"
        " from  BookGradeInfoTable  G join UserInfoTable  U  "
        " on G.userId = U.userId  "
        " order by  G.bookScore "
        + order + 
        "  , from_unixtime(G.remarkTime) desc " 
        " limit " + to_string(offset) + " , " +  to_string(count);
    
    auto buffer = conn->query<sqlComHitRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;
}


SQL_STATUS BookCommentImpl::get_comment_by_time(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse )
{//默认时间升序
    if(offset < 0 || count < 0)
        return SQL_STATUS::Illegal_info ;
    // 标题、具体内容、评分、userId、昵称、时间
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = reverse == true ? "desc":"asc" ;

    string state  = 
        " select  G.title , G.content , G.bookScore , U.userId , U.userNickName ,U.isUpdateHead, from_unixtime(G.remarkTime), G.bookId"
        " from  BookGradeInfoTable  G join UserInfoTable  U  "
        " on G.userId = U.userId  "
        " order by from_unixtime(G.remarkTime) " + 
        order + " limit " + to_string(offset) + " , " +  to_string(count);

    auto buffer = conn->query<sqlComHitRes>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookCommentImpl::get_comment_by_hit(const int & observer, const int & offset, 
                                        const int &count ,vector<commentRes> & res, const bool &reverse )
{//根据点赞数获取升序 时间降序
    if(offset < 0 || count < 0)
        return SQL_STATUS::Illegal_info ;
    // 标题、具体内容、评分、userId、昵称、时间
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string order = reverse == true ? "desc":"asc" ;
    string cond  = " order by count " + order +
                    " limit " + to_string(offset) + " , " +  to_string(count);
    
    auto buffer = conn->query<BookCommentHitCountInfoTable>(cond);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    get_full_comment(observer,buffer,res);
    if(res.size() == 0) 
        return SQL_STATUS::EXE_err;
    else 
        return SQL_STATUS::EXE_sus;

}

SQL_STATUS BookCommentImpl::get_other_info_for_count(const int & praised,inforCount & res)
{//获取其他的信息
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    //为点赞数评论查找的结果  标题、具体内容、评分、昵称,是否、更改头像、时间
    //typedef tuple<string,string,int,string,int,string>  inforCount;
    string state  = 
        " select  G.title ,G.content ,G.bookScore , U.userNickName ,U.isUpdateHead, from_unixtime(G.remarkTime) "
        " from  BookGradeInfoTable  G  "
        " join UserInfoTable  U   on G.userId = U.userId  "
        " where G.userId = " + to_string(praised);
    
    auto buffer = conn->query<inforCount>(state);
    if(buffer.size() == 0)
        return SQL_STATUS::Empty_info ;
    
    res = std::move(buffer[0]);
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookCommentImpl::hit_comment_by_bookId_praised(const int & hitter,const string & book_id,const int & praised)
{
    int isRemark = __grade->is_remark(book_id,praised);
    if(isRemark == -1)
    {//连接错误
        return SQL_STATUS::Pool_err;
    }
    else if(isRemark == 0)
    {//无对应评论
        return SQL_STATUS::Illegal_info;
    }

    BookBaseInfoTable bookbuffer ;
    SQL_STATUS ret = __book->get_book_baseInfo_by_book_id(bookbuffer,book_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    BookCommentHitInfoTable hitbuffer ;
    hitbuffer.autoBookId = bookbuffer.autoBookId ;
    hitbuffer.bookId = bookbuffer.bookId;
    hitbuffer.hitter = hitter;
    hitbuffer.praised = praised;
    //插入评论表
    ret = __comHit->insert_hit(hitbuffer);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    //统计表更新
    int isHit = __hitCount->is_existing(book_id,praised);
    if(  isHit == 1)
    {
        return __hitCount->plus_hit_count_by_bookId_praised(book_id,praised);
    }
    else if( isHit == 0)
    {//首次点赞
        return  __hitCount->insert_hit(bookbuffer.autoBookId ,book_id, praised,1);
        
    }
    else
    {
        return SQL_STATUS::Pool_err;
    }
}

SQL_STATUS BookCommentImpl::cancal_hit_comment_by_bookId_praised(const int & hitter,const string & book_id,const int & praised)
{//取消点赞
    //评论表删除
    SQL_STATUS ret = __comHit->delete_hit_by_bookId_praised(hitter,book_id,praised); 
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    //统计表更新
    int isHit = __hitCount->is_existing(book_id,praised);
    if(  isHit == 1)
    {
        return __hitCount->sub_hit_count_by_bookId_praised(book_id,praised);
    } 
    else if( isHit == 0)
    {//本无点赞
        return SQL_STATUS::Illegal_info;
    }
    else
    {
        return SQL_STATUS::Pool_err;
    }
}

SQL_STATUS BookCommentImpl::add_comment(const string &book_id ,const int &user_id,
                                        const int &book_score,const string & title,
                                        const string &content,const string & remark_time )
{
    BookBaseInfoTable buffer;
    SQL_STATUS ret = __book->get_book_baseInfo_by_book_id(buffer,book_id);
    if(ret != SQL_STATUS::EXE_sus)
        return ret;
    BookGradeInfoTable  score ;
    score.autoBookId = buffer.autoBookId ;
    score.bookId = book_id;
    score.bookScore = book_score;
    score.content = content ;
    score.remarkTime = remark_time ;
    score.userId = user_id ;
    score.title = title ;
    return __grade->insert_score(score);
}

SQL_STATUS BookCommentImpl::delete_comment(const string &book_id ,const int &user_id)
{
    int isExist = __grade->is_remark(book_id,user_id);
    if(isExist == 1)
    {
        SQL_STATUS ret = __grade->delete_score_by_bookId_userId(book_id,user_id);
        if(ret != SQL_STATUS::EXE_sus)
            return ret;
    }
    else if(isExist ==  -1)//池错误
    {
        return SQL_STATUS::Pool_err;
    }
    else//无该评论
    {
        return SQL_STATUS::Illegal_info ;
    }
    //点赞信息删除
    isExist = __hitCount->is_existing(book_id,user_id);
    if(isExist == 1)
    {
        SQL_STATUS ret = __hitCount->delete_hit_by_bookId_praised(book_id,user_id);
        if(ret != SQL_STATUS::EXE_sus)
            return ret;
        ret = __comHit->delete_hit_by_comment_by_bookId_praised(book_id,user_id);
        //完成直接返回
        return ret;
    }
    else if(isExist ==  -1)//池错误
    {
        return SQL_STATUS::Pool_err;
    }
    else//无点赞
    {
        return SQL_STATUS::EXE_sus;
    }
    
}




}



#endif


