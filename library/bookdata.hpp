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

int insert_book(const string &book_id, const string &book_name, const string &book_headurl, const string &book_down_url, const string &author_name, const string &book_type)
{
	//需要在评论前分配一个评论根ID
	BookInfoTable book{book_id, book_name, book_headurl, book_down_url, author_name, book_type, 0};
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
{ // 数据库更新图书信息
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

int up_book(const string &book_id, const string &book_name, const string &book_headurl, const string &book_downurl, const string &book_type, const string &author_name)
{
	BookInfoTable book{book_id, book_name, book_headurl, book_downurl, author_name, book_type, 0};
	return up_book(book);
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
	int sizeInt = allBook.size();
	sizeInt = sizeInt / 10;
	sizeInt += sizeInt % 10 ? 1 : 0;
	int start = curIndex < sizeInt ? 10 * curIndex : curIndex % sizeInt;
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
	int sizeInt = allBook.size();
	sizeInt = sizeInt / 10;
	sizeInt += sizeInt % 10 ? 1 : 0;
	int start = curIndex < sizeInt ? 10 * curIndex : curIndex % sizeInt;
	int end = start + 10;
	for (; start < end; ++start)
	{
		if (start < allBook.size())
		{
			res.push_back(allBook[start]);
		}
	}
	return res.size();
}
