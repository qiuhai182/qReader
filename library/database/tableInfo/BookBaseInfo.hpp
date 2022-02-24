#pragma once

#ifndef _BOOKBASEINFO_H
#define _BOOKBASEINFO_H

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
     * 书籍基本信息查询
     */
    struct BookBaseInfoTable
    {
        int autoBookId;         // 自增主键
        string bookId;			// 书籍hash码id
		string bookName;		// 书籍name
		string authorName;		// 作者名
        string bookType;		// 类型
        string publishHouse;    // 出版社
        string publishTime;     // 出版时间  
		string bookIntro ; 		// 简介
        int    bookPage;        // 页数
        int    languageType;    // 语言类型
        int    isDelete;        // 是否删除
    };
    REFLECTION(BookBaseInfoTable, autoBookId, bookId, bookName, authorName, bookType,
                publishHouse,publishTime,bookIntro,bookPage,languageType,isDelete);

    class BookBaseInfo{
    public: 
        BookBaseInfo(const bool create_status = true){
            __isCreate = create_status;
            if(!__isCreate){
                if(SQL_STATUS::EXE_sus != this->create_table())
                    throw "create BookBaseInfoTable error ";
            }
                
        }
        SQL_STATUS get_book_baseInfo_by_autoBookId(BookBaseInfoTable & book,const int & auto_book_id);
        SQL_STATUS get_autoBookId_by_bookId(const string & book_id,int & auto_book_id);
        SQL_STATUS get_all_book_baseInfo(vector<BookBaseInfoTable> &res);
        SQL_STATUS get_book_baseInfo_by_book_id(BookBaseInfoTable &book, const string &book_id);
        SQL_STATUS get_books_baseInfo_by_option(vector<BookBaseInfoTable> &books, 
                                                const option & optionName,const string &optionValue,const int & offset,const int & count) ;
        SQL_STATUS insert_book_baseInfo(const BookBaseInfoTable &book);
        SQL_STATUS insert_book_baseInfo(const int & auto_book_id,const string &book_id, 
                                                const string &book_name, const string &author_name, 
                                                const string &bookType, const string &publishHouse,
                                                const string &publishTime, const string & bookIntro,
                                                const int & book_page,const int & languageType);
        SQL_STATUS del_book_baseInfo(const string &book_id);
        SQL_STATUS up_book_baseInfo(std::map<option,sqlUpdateVal> up_data);
        bool isOption(const option & opt);
        SQL_STATUS get_book_baseInfo_by_offset(vector<BookBaseInfoTable> & books , 
                                                const int & offset,const int & count);
    private:
        //是否删除  -1:不存在 0:未删除 1:删除
        SQL_STATUS repeat_add_book(const int & auto_book_id);
        int isDelete(const string & book_id);
        SQL_STATUS create_table();
        SQL_STATUS create_autoBookId_index();
        SQL_STATUS create_indexs();
        bool __isCreate;
    };




/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍基本信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS BookBaseInfo::create_table()
{
    /*创建书籍信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;


    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << "  conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    conn->execute("DROP TABLE IF EXISTS BookBaseInfoTable");
    string state = 
    "CREATE TABLE BookBaseInfoTable( "
        " autoBookId INTEGER NOT NULL AUTO_INCREMENT , "
        " bookId text NOT NULL, "
        " bookName text NOT NULL , "
        " authorName text NOT NULL, "
        " bookType  text NOT NULL, "  
        " publishHouse text NOT NULL, "       
        " publishTime text NOT NULL, "  
        " bookIntro text NOT NULL, "
        " bookPage INTEGER NOT NULL, " 
        " languageType INTEGER NOT NULL, "
        " isDelete INTEGER  DEFAULT 0 ,"
        " PRIMARY KEY (autoBookId) "
    " ) ENGINE = InnoDB AUTO_INCREMENT = 10000000 DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        cout<<" 创建成功"<<endl;
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else{
        cout<<" 创建失败 "<<endl;
        return SQL_STATUS::EXE_err;
    }
        
}

SQL_STATUS BookBaseInfo::create_autoBookId_index()
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
    state = "ALTER TABLE BookBaseInfoTable ADD INDEX autoBookId_index (autoBookId)" ;
    return execute_sql(conn,"create BookBaseInfo  autoBookId index",state);
}

SQL_STATUS BookBaseInfo::create_indexs()
{
    return create_autoBookId_index();
}

SQL_STATUS BookBaseInfo::get_book_baseInfo_by_autoBookId(BookBaseInfoTable & book,const int & auto_book_id)
{// 通过主键id获取书籍信息
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = " where autoBookId = " + to_string(auto_book_id) + " and isDelete =  0";
	auto res = conn->query<BookBaseInfoTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	else
		book = std::move(res[0]);
	return SQL_STATUS::EXE_sus;

}

SQL_STATUS BookBaseInfo::repeat_add_book(const int & auto_book_id)
{//添加已经设置删除的书籍
    
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
    string state = "update BookBaseInfoTable "
                    "  set isDelete = 0 " 
                    " where autoBookId = " + to_string(auto_book_id );

	return execute_sql(conn,"repeat add book base ",state);

}

int BookBaseInfo::isDelete(const string & book_id)
{//是否删除  -2: 连接池错误-1:不存在 0:未删除 1:删除
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return -2;
	}
    string state = " where bookId = \'" + book_id + "\'";
	auto res = conn->query<BookBaseInfoTable>(state);
	if (res.size() == 0)
		return -1;
	else if(res[0].isDelete == 0)
		return 0;
    else    
        return 1 ;
}

SQL_STATUS BookBaseInfo::get_autoBookId_by_bookId(const string & book_id,int & auto_book_id)
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

    string state = "where bookId = \'" + book_id + "\' and  isDelete = 0" ;
	auto res = conn->query<BookBaseInfoTable>(state);
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    auto_book_id =res[0].autoBookId;
	return SQL_STATUS::EXE_sus;


}

SQL_STATUS BookBaseInfo::get_all_book_baseInfo(vector<BookBaseInfoTable> &res)
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "  conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	res = std::move(conn->query<BookBaseInfoTable>());
	if(res.size() == 0)
        return SQL_STATUS::EXE_err;
    else
        return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookBaseInfo::get_book_baseInfo_by_book_id(BookBaseInfoTable &book, const string &book_id)
{ // 通过书籍id获取书籍信息
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond = "where bookId = \'" + book_id + "\'";
	auto res = conn->query<BookBaseInfoTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	else
		book = std::move(res[0]);
	return SQL_STATUS::EXE_sus;
}

bool BookBaseInfo::isOption(const option & opt)
{
    if( opt == "bookName" || opt == "authorName" || opt == "publishHouse" || opt == "bookType")
        return true;
    else 
        return false;
}

SQL_STATUS BookBaseInfo::get_books_baseInfo_by_option(vector<BookBaseInfoTable> &books, 
                                                const option & optionName,const string &optionValue,const int & offset,const int &count) 
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "  conn is NULL  "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
    if( !isOption(optionName))
        return SQL_STATUS::Illegal_info ;
	string cond = " where " + optionName + " LIKE \'\%" +  optionValue + "\%\' 
                    and isDelete = 0 limit " + to_string(offset) + " , " +  to_string(count);
	auto res = conn->query<BookBaseInfoTable>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
	// for (auto &i : res)
	// {
	// 	books.push_back(i);
	// }
    books = std::move(res);
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookBaseInfo::get_books_baseInfo_by_option_downloadCount(vector<BookBaseInfoTable> &books, 
                                                const option & optionName,const string &optionValue,const int & offset,const int &count) 
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << "  conn is NULL  "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
    if(!isOption(optionName))
        return SQL_STATUS::Illegal_info ;
    string cond = "select base.* from BookBaseInfoTable base, BookDownloadCountTable bdc " + 
                    "where base.autoBookId = bdc.autoBookId and base." + optionName + " LIKE \'\%" +  optionValue + 
                    "\%\' and base.isDelete = 0 order by bdc.times limit " + to_string(offset) + " , " +  to_string(count) + " ORDER BY bdc.times DESC";
    auto res = conn->query<std::tuple<int, string, string, string, string, string, string, string, int, int, int>>(cond);
	if (res.size() == 0)
		return SQL_STATUS::EXE_err;
    for(auto &i : res) {
        BookBaseInfoTable book(get<0>(i).autoBookId, get<1>(i).bookId, get<2>(i).bookName, get<3>(i).authorName,
        get<4>(i).bookType, get<5>(i).publishHouse, get<6>(i).publishTime, get<7>(i).bookIntro, get<8>(i).bookPage,
        get<9>(i).languageType, get<10>(i).isDelete);
        books.append(book);
    }
	return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookBaseInfo::insert_book_baseInfo(const BookBaseInfoTable &book)
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

	string bookid = book.bookId;
	BookBaseInfoTable bufBook;

    int ret = isDelete(book.bookId);
    cout<<"    ee  "<<ret<<endl;
	if( 1 == ret )
    {
        return repeat_add_book(book.autoBookId);
    }
    else if(-1 == ret)
	{
        cout<<" autoBookId "<<book.autoBookId<<" bookName "<<book.bookName
            <<" bookType "<<book.bookType<<" bookla "<<book.languageType
            <<" bookPage "<<book.bookPage<<" bookputi "<<book.publishTime
            <<" bookhouse  "<<book.publishHouse<< "  isde"<<book.isDelete <<endl;
		if (conn->insert(book) != 1)
		{
			cout << __FILE__ << " : " << __LINE__ 
                << "insert bookBaseInfo error" << endl;
			return SQL_STATUS::EXE_err;
		}
		else
			return SQL_STATUS::EXE_sus;
	}
    else
    {
        return SQL_STATUS::EXE_err;
    }
}

SQL_STATUS BookBaseInfo::insert_book_baseInfo(const int & auto_book_id,const string &book_id, 
                                                const string &book_name, const string &author_name, 
                                                const string &bookType,const string &publishHouse,
                                                const string &publishTime,const string & bookIntro,
                                                const int & book_page,const int & languageType)
{
    BookBaseInfoTable book{auto_book_id,book_id, 
                            book_name,author_name, 
                            bookType,publishHouse , 
                            publishTime,bookIntro,
                            book_page,languageType,
                            0};
	return insert_book_baseInfo(book);
}

SQL_STATUS BookBaseInfo::del_book_baseInfo(const string &book_id)
{// 数据库删除书籍基本信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
    string cond = "where bookId = \'" +  book_id + "\'";
	auto res = conn->query<BookBaseInfoTable>(cond);
	if ( 0 != isDelete(res[0].bookId))
		return SQL_STATUS::EXE_err; //书籍不存在  已经删除
	else
	{
        string state = "update BookBaseInfoTable "
                        "  set isDelete = 1 " 
                        " where bookId = \'" +  book_id + "\' ";
		return execute_sql(conn,"delete book base ",state);
	}

}

SQL_STATUS BookBaseInfo::up_book_baseInfo(std::map<option,sqlUpdateVal> up_data)
{//更改基础信息

    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ 
			 << "  conn is NULL  "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}else{
        if(up_data.size() == 0)
            return SQL_STATUS::Illegal_info;
    }

    string state ,buffer,cond;
    state = "update BookBaseInfoTable set ";

    for(auto up : up_data)
    {
        if(up.first == "bookId"){
            cond = up.second.strVal;
            continue;
        }
        if( up.second.type == COND_TYPE::Int )
        {
            buffer = " " + up.first + " =  " +  std::to_string(up.second.intVal) + "  ,";
        }else{
            buffer = " " + up.first + " =  \'" +  up.second.strVal  + "\' ,";
        }
        state += buffer;
    }
    //去除多余的,
    state.replace(state.rfind(","), 1, "");
    //添加条件
    state += " where bookId = \'" + cond + "\'";
    cout<<"  state  is "<<state<<endl;
    return execute_sql(conn,"update book base information",state);

}

SQL_STATUS BookBaseInfo::get_book_baseInfo_by_offset(vector<BookBaseInfoTable> & books , 
                                                const int & offset, const int & count)
{ // 在表中查询指定偏移量、最大结果数量的书籍
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ 
			 << " conn is NULL "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string cond 
		//=  "limit " +  to_string(count) +   " OFFSET " + to_string(offset) ; ;
        =  " where isDelete = 0  limit " +  to_string(offset) +   " , " + to_string(count) ; 
	auto res = conn->query<BookBaseInfoTable>(cond);
	if (res.size() == 0)
	{
		cout << "FILE: " << __FILE__ 
			 << " offset read  error "
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::EXE_err;
	}
	else
	{
		books = std::move(res);
		return SQL_STATUS::EXE_sus;
	}
}



}

#endif
