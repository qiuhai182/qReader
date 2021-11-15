#pragma once

#include <iostream>
#include <vector>
#include "mysql_tables.hpp"
using namespace ormpp;
using namespace std;


// 封装书籍信息数据库操作函数: 查询、修改、删除


int get_all_book(vector<BookInfoTable> &res)
{ // 获取数据库所有书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	res = conn->query<BookInfoTable>();
	return res.size();
}

int get_book_by_id(BookInfoTable &book, const string &book_id)
{ // 通过书籍id获取书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where bookId = \"" + book_id + "\"";
	auto res = conn->query<BookInfoTable>(cond);
	if (res.size() == 0)
		return -1;
	else
		book = res[0];
	return res.size();
}

int get_book_by_bookname(vector<BookInfoTable> &books, const string &book_name)
{ // 通过书籍名获取书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where bookName LIKE \'\%" + book_name + "\%\'";
	auto res = conn->query<BookInfoTable>(cond);
	if (res.size() == 0)
		return -1;
	for (auto &i : res)
	{
		books.push_back(i);
	}
	return res.size();
}

int get_book_by_authorname(vector<BookInfoTable> &books, const string &author_name)
{ // 通过书籍作者获取书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where authorName LIKE \"\%" + author_name + "\%\"";
	
	auto res = conn->query<BookInfoTable>(cond);
	if (res.size() == 0)
		return -1;

	for (auto &i : res)
	{
		books.push_back(i);
	}
	return res.size();
}

/*
 *	bookId为Key，bookId为一本书的md5值
 *	如果md5值不同，则表示唯一
 *	
 */
int insert_book(const BookInfoTable &book)
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

	string bookid = book.bookId;
	BookInfoTable bufBook;
	if (1 == get_book_by_id(bufBook, bookid))
		return -2; // 已存在该书籍
	else
	{
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn->insert(book) != 1)
		{
			cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
			return 0;
		}
		else
			return 1;
	}
}

int insert_book(const string &book_id, const string &book_name, 
		const string &book_headurl, const string &book_down_url, 
		const string &author_name, const string &book_type,
		const string & book_intro)
{
	//需要在评论前分配一个评论根ID
	BookInfoTable book{book_id, book_name, book_headurl, 
					book_down_url, author_name, book_type,
					 0,book_intro};
	return insert_book(book);
}

int del_book(const string &book_id)
{ // 数据库删除书籍
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where bookId = \"" + book_id +"\"";
	auto res = conn->query<BookInfoTable>(cond);
	if (res.size() == 0)
		return -1; //书籍不存在
	else
	{
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		string cond = "bookId = " + book_id;
		if (conn->delete_records<BookInfoTable>(cond))
			return 1;
		else
			return -1;
	}
}

int up_book(const BookInfoTable &book)
{ // 数据库更新图书信息  暂时废弃
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	if (conn->update(book) != 1)
	{
		cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
		return 0;
	}
	return 1;
}

int up_book(const string &book_id, const string &book_name, 
			const string &book_headurl, const string &book_downurl, 
			const string &book_type, const string &author_name,
			const string & book_intro)
{

	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string bookNameChange= book_name 	== "" 	? "" : " bookName = \'"  	+ book_name + "\' , ";
	string headUrlChange = book_headurl == "" 	? "" : " bookHeadurl = \'" 	+ book_headurl + "\' , ";
	string downUrlChange = book_downurl == "" 	? "" : " bookDownurl = \'" 	+ book_downurl + "\' , ";
	string typeChange	 = book_type 	== "" 	? "" : " bookType = \'" 	+ book_type + "\' , ";
	string authorChange  = author_name 	== "" 	? "" : " authorName = \'" 	+ author_name + "\' , ";
	string introChange   = book_intro 	== "" 	? "" : " bookIntro = \'" 	+ book_intro + "\'  ";
	string cond = "update BookInfoTable set " + bookNameChange 
				+ headUrlChange + downUrlChange
				+ typeChange    + authorChange + introChange
				+ " where bookId = \'" + book_id + "\'";
				cout <<"cond is "<<cond <<endl ;
	if (conn->execute(cond) == INT_MIN)
	{
		LOG(WARNING) << __FILE__ << " : " << __LINE__ << "insert error" << endl;
		return 0;
	}
	else{
		return 1;
	}

}

int get_all_ads(vector<BookADSTable> &res)
{ // 获取数据库所有书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	res = conn->query<BookADSTable>();
	return res.size();
}

int get_recommend_book(vector<BookInfoTable> &res, int curIndex)
{ // 获取个性化推荐书籍
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	auto allBook = conn->query<BookInfoTable>();
	int sizeInt = 0;
	sizeInt = allBook.size() / 10;
	sizeInt += allBook.size() % 10 ? 1 : 0;
	int start = curIndex < sizeInt ? 10 * curIndex : (curIndex % sizeInt) * 10;
	int end = start + 10;
	for(; start < end; ++start)
	{
		if (start < allBook.size())
		{
			res.push_back(allBook[start]);
		}
	}
	return res.size();
}

int get_browse_book(vector<BookInfoTable> &res, int curIndex)
{ // 浏览书城时随机推荐书籍
	return get_recommend_book(res, curIndex);
}


int get_book_offset(vector<BookInfoTable> & books , const int & offset,const int & count)
{//在信息表读取区域的图书
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond 
		=  "limit " +  to_string(count) +   " OFFSET " + to_string(offset) ; ;
	auto res = conn->query<BookInfoTable>(cond);
	if (res.size() == 0)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "mostly search error"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	else
	{
		books = std::move(res);
		return books.size();
	}
}

int get_mostly_search_by_month_count(const string & monthTime,vector<SearchStatisticsTable> &searchList,const int & count)
{//在搜索表格获取本月中搜索最多count本
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond 
		=  "where dayTime LIKE \"\%" + monthTime + "\%\" order by  times  desc limit " + std::to_string(count); ;
	auto res = conn->query<SearchStatisticsTable>(cond);
	if (res.size() == 0)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "mostly search error"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	else
	{
		searchList = std::move(res);
		return searchList.size();
	}
}

int get_searchStatistics_info_by_id_and_daytime(const string &bookId, const string &dayTime,SearchStatisticsTable & stat)
{//通过bookid,daytime获得次数
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING)  << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond 
		= "where bookId = \'" + bookId + "\' and  dayTime = \'" + dayTime + "\'";
	auto res = conn->query<SearchStatisticsTable>(cond);
	if (res.size() == 0)
		return -1;
	else
		stat = std::move(res[0]);
	return res.size();
}

int insert_seacrh_inf(const SearchStatisticsTable & stat)
{//插入新的搜索信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if(conn == NULL)
	{
		LOG(WARNING)  << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -2;
	}
	if(conn->insert(stat) != 1)
	{
		LOG(WARNING) <<__FILE__ << " : " << __LINE__ << "SearchStatistics insert error"<<endl ;
		return -1 ;
	}
	else
	{
		return 1 ;
	}
}

int update_seacrh_inf(const SearchStatisticsTable & stat)
{//更新书本当天的搜索信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if(conn == NULL)
	{
		LOG(WARNING)  << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -2;
	}
	string cond = "update SearchStatisticsTable set  times = " 
			+ to_string(stat.times) + "  where bookId = \'" + stat.bookId 
			+ "\'  and  dayTime  = \'" + stat.dayTime + "\'" ;
	if(conn->execute(cond) == INT_MIN)
	{
		LOG(WARNING) <<__FILE__ << " : " << __LINE__ << "SearchStatistics update error"<<endl ;
		return -1 ;
	}
	else
	{
		return 1 ;
	}
}

int plus_search_times(const string & book_id,const string & dayTime,const string & bookName)
{//搜索时更新搜索时间
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING)  << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -2;
	}
	SearchStatisticsTable stat{book_id, dayTime,1,bookName}; ;
	if( get_searchStatistics_info_by_id_and_daytime(book_id, dayTime,stat) == -1)
	{//一天该书新搜索
		if(insert_seacrh_inf(stat) == 1){
			return 1 ;
		}else{
			return -1;
		}
	}
	else//搜索加一
	{ 
		stat.times += 1;
		if(update_seacrh_inf(stat) == 1){
			return 1 ;
		}else{
			return -1;
		}
	}

}