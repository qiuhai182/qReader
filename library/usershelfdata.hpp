#pragma once

#include <iostream>
#include <vector>
#include "mysql_tables.hpp"
using namespace ormpp;
using namespace std;


// 封装书架书籍信息数据库操作函数


int get_book_by_userid(vector<BookInfoTable> &books, const string &userId)
{ // 通过用户id查询书架书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where userId = \"" + userId + "\"";
	vector<UserShelfTable> shelfBooks = conn->query<UserShelfTable>(cond);
	if (shelfBooks.size() == 0)
	{
		return -1;
	}
	else
	{
		for (auto i : shelfBooks)
		{
			BookInfoTable temp;
			int res = get_book_by_id(temp, i.bookId);
			if(res)
				books.push_back(temp);
		}
	}
	return books.size();
}

int get_shelftable_by_userid(vector<UserShelfTable> &books, const string &userId)
{ // 通过用户id查询书架书籍信息，返回简易版本 废弃
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where userId = \"" + userId + "\"";
	vector<UserShelfTable> shelfBooks = conn->query<UserShelfTable>(cond);
	if (shelfBooks.size() == 0)
	{
		return -1;
	}
	else
	{
		books = shelfBooks;
		// for (auto i : shelfBooks)
		// {
		// 	UserShelfTable temp;
		// 	int res = get_book_by_id(temp, i.bookId);
		// 	if(res)
		// 		books.push_back(temp);
		// }
	}
	return books.size();
}


int del_shelf_book(const string &userId, const string &bookId)
{ // 数据库删除用户书架里书籍
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string userId_bookId = "" + userId + "_"+ bookId;
	string cond = "userId_bookId = " + userId_bookId;
	if (conn->delete_records<UserShelfTable>(cond))
		return 1;
	else
		return 0;
}

int insert_shelf(const UserShelfTable &book)
{ // 增加书籍信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	del_shelf_book(book.userId, book.bookId);
	if (conn->insert(book) != 1)
	{
		cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
		return 0;
	}
	return 1;
}


int insert_shelf(const string &userId, const string &bookId)
{ // 增加书籍信息
	//需要在评论前分配一个评论根ID
	UserShelfTable book{"" + userId + "_" +bookId, userId, bookId};
	return insert_shelf(book);
}



