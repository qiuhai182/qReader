#pragma once

#ifndef _BOOKGRADEINFO_H
#define _BOOKGRADEINFO_H

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

#define SCORE_NUM  5  //即分数粒度 1为一个分段
#define SCORE_INTERVAL 1 //数据库存储的为整数对应为10

using namespace ormpp;
using namespace std;
typedef string option;

namespace ormpp
{
  /** 
     * 书籍评分信息查询
     */
    struct BookGradeInfoTable
    {
        int autoBookId ;        // 外键
        string bookId ;         // bookId
        int userId;             // 评论ID 主键之一
        int bookScore;          // 书籍评分
        string title;           // 标题
        string content;         // 内容
        string remarkTime;      // 评分时间
    };
    REFLECTION(BookGradeInfoTable, autoBookId,bookId,userId, bookScore,
                        title,content,remarkTime);
    
    /**
     * 单本书籍评分统计
     */
    struct BookScoreStat
    {
        double avgScore;       // 综合评分
        int count;          // 评分人数
    };


    class BookGradeInfo{
    public: 
        BookGradeInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create BookGradeInfoTable error ";
            }
        }
        SQL_STATUS get_score_seg_stat(const int & score,const string &book_id, int & number);
        SQL_STATUS get_average_score(double & average,const int & auto_book_id);
        SQL_STATUS get_remark_count(int & count,const int & auto_book_id);
        SQL_STATUS get_grade_by_double_id(const int & auto_book_id,
                                        const int& user_id,BookGradeInfoTable & grade);
        SQL_STATUS update_score(const BookGradeInfoTable & score);
        SQL_STATUS delete_score_by_bookId_userId(const string &book_id ,const int &user_id);
        SQL_STATUS insert_score(const BookGradeInfoTable & score);
        SQL_STATUS insert_score(const int & auto_book_id,
                                        const string &book_id ,const int &user_id,
                                        const int &book_score,const string & title,
                                        const string &content,const string & remark_time );
        int is_remark(const string &book_id ,const int &user_id);
        /*占位 将实现*/
        // virtual SQL_STATUS del_book();
        // virtual SQL_STATUS up_book();
        // virtual SQL_STATUS up_book();

    private:
        SQL_STATUS create_table();
        SQL_STATUS create_double_index();
        SQL_STATUS create_indexs();
        bool __isCreate;
    };

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍评分信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS BookGradeInfo::update_score(const BookGradeInfoTable & score)
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

    int ret = conn->update(score);
    if( 1 == ret)
        return SQL_STATUS::EXE_sus;
    else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS BookGradeInfo::insert_score(const BookGradeInfoTable & score)
{//插入评分
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

	//string bookid = score.bookId;
	BookGradeInfoTable bufBook;
	if (SQL_STATUS::EXE_sus == get_grade_by_double_id(score.autoBookId, score.userId,bufBook) )
    {//覆盖
        if(SQL_STATUS::EXE_sus == update_score(score))
            return SQL_STATUS::EXE_err;
    } 
	else
	{
		if (conn->insert(score) != 1)
		{
			cout << __FILE__ << " : " << __LINE__ 
                << "insert BookGradeInfoTable  error" << endl;
			return SQL_STATUS::EXE_err;
		}
		else
			return SQL_STATUS::EXE_sus;
	}

}

SQL_STATUS BookGradeInfo::insert_score(const int & auto_book_id,
                                        const string &book_id ,const int &user_id,
                                        const int &book_score,const string & title,
                                        const string &content,const string & remark_time )
{
    BookGradeInfoTable score;
    score.autoBookId = auto_book_id;
    score.bookId = book_id;
    score.bookScore = book_score;
    score.userId = user_id;
    score.title = title;
    score.content = content;
    score.remarkTime = remark_time;
    return insert_score(score);
}

SQL_STATUS BookGradeInfo::get_score_seg_stat(const int & score,const string &book_id, int & number)
{//获取分段统计
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "select count(bookScore) from BookGradeInfoTable "
                    " group by bookScore ,bookId  having "
                    " bookScore = " + to_string(score ) + 
                    " and bookId = \'" + book_id + "\'";

	auto res = conn->query<std::tuple<int>>(state);
    if(res.size() == 0){
        number = 0;
    }
    else
    {
        number = (int)std::get<0>(res[0]) ;
    }
    
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookGradeInfo::get_average_score(double & average,const int & auto_book_id)
{//获取平均分
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "select avg(bookScore) from BookGradeInfoTable "
                    " group by autoBookId  having "
                    " autoBookId = " + to_string(auto_book_id ) ;
	auto res = conn->query<std::tuple<int>>(state);
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    average = std::get<0>(res[0]) ;
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookGradeInfo::get_remark_count(int & count,const int & auto_book_id)
{//获取评论人数
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "select count(autoBookId) from BookGradeInfoTable "
                    " group by autoBookId  having " 
                    " autoBookId = "  + to_string(auto_book_id ) ;
	auto res = conn->query<std::tuple<int>>(state);
    if(res.size() == 0){
        count = 0;
    }   
    else
    {
        count = (int)std::get<0>(res[0]) ;
    }
    
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookGradeInfo::delete_score_by_bookId_userId(const string &book_id ,const int &user_id)
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
    
    string cond = " bookId = \'" + book_id + "\'  and userId = " + to_string(user_id);
    bool ret = conn->delete_records<BookGradeInfoTable>(cond);
    if(ret)
        return SQL_STATUS::EXE_sus;
    else
        return SQL_STATUS::EXE_err;
    
}

SQL_STATUS BookGradeInfo::get_grade_by_double_id(const int & auto_book_id,const int & user_id,BookGradeInfoTable & grade)
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

    string state = "where autoBookId = " + std::to_string(auto_book_id) +
                    " and  userId = " + std::to_string(user_id);
	auto res = conn->query<BookGradeInfoTable>(state);
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    grade = res[0];
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookGradeInfo::create_table()
{
    /*创建书籍评分信息表 */
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

    conn->execute("DROP TABLE IF EXISTS BookGradeInfoTable");
    string state = 
    "CREATE TABLE BookGradeInfoTable( "
        " autoBookId INTEGER NOT NULL, "
        " bookId TEXT NOT NULL, "
        " userId INTEGER NOT NULL , "
        " bookScore INTEGER NOT NULL DEFAULT 0 , "
        " title TEXT NOT NULL ,"
        " content TEXT NOT NULL ,"
        " remarkTime TEXT NOT NULL, "  
        " PRIMARY KEY (autoBookId,userId) ,"
        " CONSTRAINT grade_book_id FOREIGN KEY (autoBookId) REFERENCES  BookBaseInfoTable(autoBookId) "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS BookGradeInfo::create_double_index()
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
    state = "ALTER TABLE BookGradeInfoTable ADD INDEX double_index (autoBookId,userId)" ;
    return execute_sql(conn,"create BookGradeInfoTable  double_index ",state);
}

SQL_STATUS BookGradeInfo::create_indexs()
{
    return create_double_index();
}

int BookGradeInfo::is_remark(const string &book_id ,const int &user_id)
{// -1:池错误  0:在 1:在
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return -1;
    }
    string cond = " where bookId = \'" + book_id + "\' and userId =  " +to_string(user_id) ;
    auto res = conn->query<BookGradeInfoTable>(cond);

    if(res.size() == 0)
        return 0;
    else
        return 1;
}



}




#endif
