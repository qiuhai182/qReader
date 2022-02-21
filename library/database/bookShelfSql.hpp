#pragma once

#ifndef _USESQL_H
#define _USESQL_H

#include <iostream>
#include <thread>
#include <map>
#include <memory>
#include <typeinfo>
#include "tableInfo/BookBaseInfo.hpp"
#include "tableInfo/BookGradeInfo.hpp"
#include "tableInfo/BookShelfInfo.hpp"
#include "tableInfo/BookSearchInfo.hpp"
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"



using namespace ormpp;
using namespace std;
typedef string option;

namespace ormpp
{
    

    class BookShelfInfoImpl
    {
    public:
        BookShelfInfoImpl()
        {
            __shelf = new UserShelfInfo;
        }
        ~BookShelfInfoImpl()
        {
            delete __shelf;
        }
        SQL_STATUS get_book_by_userid(vector<UserShelfTable> &books, const int &user_id);
        SQL_STATUS remove_shelf_book(const int  &user_id, const string &book_id);
        SQL_STATUS insert_shelf(const UserShelfTable &book);
        SQL_STATUS insert_shelf(const int  &user_id, const string &book_id,
                                        const int & auto_book_id,const string &add_time);
    private:
        UserShelfInfo * __shelf;
    };

}

SQL_STATUS BookShelfInfoImpl::get_book_by_userid(vector<UserShelfTable> &books, const int &user_id)
{
    return __shelf->get_book_by_userid(books, user_id);
}

SQL_STATUS BookShelfInfoImpl::remove_shelf_book(const int &user_id, const string &book_id)
{
    return __shelf->remove_shelf_book(user_id, book_id);
}

SQL_STATUS BookShelfInfoImpl::insert_shelf(const UserShelfTable &book)
{
    return __shelf->insert_shelf(book);
}

SQL_STATUS BookShelfInfoImpl::insert_shelf(const int &user_id, const string &book_id,
                                const int & auto_book_id,const string &add_time)
{
    return __shelf->insert_shelf(user_id,book_id,auto_book_id,add_time);
}

#endif
