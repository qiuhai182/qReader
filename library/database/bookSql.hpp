#pragma once

#ifndef _BOOKSQL_H
#define _BOOKSQL_H

#include <tuple>
#include <iostream>
#include <thread>
#include <memory>
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"
#include "../bookType.hpp"
#include "tableInfo/BookBaseInfo.hpp"
#include "tableInfo/BookGradeInfo.hpp"
#include "tableInfo/BookSearchInfo.hpp"
#include "tableInfo/BookDownloadCount.hpp"

using namespace ormpp;
using namespace std;
typedef std::string option;

namespace ormpp
{
    typedef tuple<BookBaseInfoTable, BookScoreStat> CombineBook;
    struct BookADSTable; //占位
    class BookInfoImpl
    {
        // 连接查询得到的结构
        typedef tuple<int, string, string, string, string, string, string, string, int, int> Join_Book;

    public:
        BookInfoImpl()
        {
            __base = new BookBaseInfo(true);
            __grade = new BookGradeInfo(true);
            __search = new BookSearchInfo(true);
            __downloadCount = new BookDownloadCountTable(true);
        }
        ~BookInfoImpl()
        {
            delete __base;
            delete __grade;
            delete __search;
            delete __downloadCount;
        }
        SQL_STATUS get_book_by_autoBookId(CombineBook &book, const int &auto_book_id);
        SQL_STATUS get_book_by_book_id(CombineBook &book, const string &book_id);
        SQL_STATUS get_book_by_book_id(BookDownloadCountTable &downloadCount, const string &bookId);
        SQL_STATUS get_books_by_option(vector<CombineBook> &books, const option &optionName,
                                       const string &optionValue, const int &offset, const int &count);
        SQL_STATUS set_newest_book_count(BookDownloadCountTable &downloadCount);
        SQL_STATUS insert_book_info(const BookBaseInfoTable &book);
        SQL_STATUS insert_book_info(const int &auto_book_id, const string &book_id,
                                    const string &book_name, const string &author_name,
                                    const string &bookType, const string &publishHouse,
                                    const string &publishTime, const string &bookIntro,
                                    const int &book_page, const int &languageType);
        SQL_STATUS get_all_book_info(vector<CombineBook> &books);
        virtual SQL_STATUS up_book_Info(std::map<option, sqlUpdateVal> up_data);
        /*占位 将实现*/
        // virtual SQL_STATUS del_book();
        // virtual SQL_STATUS up_book();
        // virtual SQL_STATUS up_book();
        SQL_STATUS get_mostly_search_by_month_count(const string &monthTime,
                                                    vector<SearchStatisticsTable> &searchList, const int &count);
        SQL_STATUS get_searchStatistics_info_by_id_and_daytime(const string &bookId,
                                                               const string &dayTime, SearchStatisticsTable &stat);
        SQL_STATUS insert_seacrh_inf(const SearchStatisticsTable &stat);
        SQL_STATUS update_seacrh_inf(const SearchStatisticsTable &stat);
        SQL_STATUS plus_search_times(const int &auto_book_id, const string &book_id,
                                     const string &dayTime, const string &bookName);
        SQL_STATUS get_all_ads(vector<BookADSTable> &res);
        SQL_STATUS get_recommend_book(vector<CombineBook> &books,
                                      const int &curIndex);
        SQL_STATUS get_books_by_fuzzy(vector<CombineBook> &books,
                                      const string &words);
        SQL_STATUS get_browse_book(vector<CombineBook> &books,
                                   const int &curIndex);
        SQL_STATUS get_book_offset(vector<CombineBook> &books,
                                   const int &offset, const int &count);

    private:
        BookBaseInfo *__base;
        BookGradeInfo *__grade;
        BookSearchInfo *__search;
        BookDownloadCountTable *__downloadCount;
    };

}

/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@书籍关联操作@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS BookInfoImpl::get_book_by_autoBookId(CombineBook &book,
                                                const int &auto_book_id)
{
    BookBaseInfoTable base_book_info;
    SQL_STATUS ret = __base->get_book_baseInfo_by_autoBookId(base_book_info, auto_book_id);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;

    BookScoreStat stat_info;

    ret = __grade->get_remark_count(stat_info.count, base_book_info.autoBookId);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;

    //暂无评分
    if (stat_info.count == 0)
    {
        stat_info.avgScore = -1;
    }
    else
    {
        ret = __grade->get_average_score(stat_info.avgScore, base_book_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            return SQL_STATUS::EXE_err;
    }

    /*获取数据完成*/
    CombineBook buffer(std::move(base_book_info), std::move(stat_info));
    book = std::move(buffer);

    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_all_book_info(vector<CombineBook> &books)
{
    vector<BookBaseInfoTable> base_info_books;
    SQL_STATUS ret = __base->get_all_book_baseInfo(base_info_books);
    if (ret == SQL_STATUS::EXE_err || ret == SQL_STATUS::Pool_err)
        return SQL_STATUS::EXE_err;
    else if (ret == SQL_STATUS::Illegal_info)
        return SQL_STATUS::Illegal_info;
    //合法可进行下一步
    for (auto base_info : base_info_books)
    {
        BookScoreStat stat_info;
        ret = __grade->get_remark_count(stat_info.count, base_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            continue;
        //暂无评分
        if (stat_info.count == 0)
        {
            stat_info.avgScore = -1;
        }
        else
        {
            ret = __grade->get_average_score(stat_info.avgScore, base_info.autoBookId);
            if (ret != SQL_STATUS::EXE_sus)
                continue;
        }
        CombineBook buffer(std::move(base_info), std::move(stat_info));
        books.push_back(buffer);
    }

    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_book_by_book_id(CombineBook &book, const string &book_id)
{
    BookBaseInfoTable base_book_info;
    SQL_STATUS ret = __base->get_book_baseInfo_by_book_id(base_book_info, book_id);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;
    BookScoreStat stat_info;
    ret = __grade->get_remark_count(stat_info.count, base_book_info.autoBookId);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;
    //暂无评分
    if (stat_info.count == 0)
    {
        stat_info.avgScore = -1;
    }
    else
    {
        ret = __grade->get_average_score(stat_info.avgScore, base_book_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            return SQL_STATUS::EXE_err;
    }
    /*获取数据完成*/
    CombineBook buffer(std::move(base_book_info), std::move(stat_info));
    book = std::move(buffer);
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_book_by_book_id(BookDownloadCountTable &downloadCount, const string &bookId)
{
    BookDownloadCountTable download_book_count;
    ret = __downloadCount->get_newest_count_by_id(downloadCount, bookId);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;
    BookBaseInfoTable base_book_info;
    SQL_STATUS ret = __base->get_book_baseInfo_by_book_id(base_book_info, bookId);
    if (ret != SQL_STATUS::EXE_sus)
        return SQL_STATUS::EXE_err;
    downloadCount.autoBookId = base_book_info.autoBookId;
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_books_by_option(vector<CombineBook> &books, const option &optionName,
                                             const string &optionValue, const int &offset, const int &count)
{
    if (offset < 0 || count < 0)
        return SQL_STATUS::Illegal_info;
    vector<BookBaseInfoTable> base_info_books;
    SQL_STATUS ret = __base->get_books_baseInfo_by_option(base_info_books, optionName, optionValue, offset, count);
    if (ret == SQL_STATUS::EXE_err || ret == SQL_STATUS::Pool_err)
        return SQL_STATUS::EXE_err;
    else if (ret == SQL_STATUS::Illegal_info)
        return SQL_STATUS::Illegal_info;
    //合法可进行下一步
    for (auto base_info : base_info_books)
    {
        BookScoreStat stat_info;
        ret = __grade->get_remark_count(stat_info.count, base_info.autoBookId);

        if (ret != SQL_STATUS::EXE_sus)
            continue;
        //暂无评分
        if (stat_info.count == 0)
        {
            stat_info.avgScore = -1;
        }
        else
        {
            ret = __grade->get_average_score(stat_info.avgScore, base_info.autoBookId);
            if (ret != SQL_STATUS::EXE_sus)
                continue;
        }

        CombineBook buffer(std::move(base_info), std::move(stat_info));
        books.push_back(buffer);
    }

    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::set_newest_book_count(BookDownloadCountTable &downloadCount)
{
    return __downloadCount->set_newest_count(downloadCount);
}


SQL_STATUS BookInfoImpl::insert_book_info(const BookBaseInfoTable &book)
{
    return __base->insert_book_baseInfo(book);
}

SQL_STATUS BookInfoImpl::up_book_Info(std::map<option, sqlUpdateVal> up_data)
{
    return __base->up_book_baseInfo(up_data);
}

SQL_STATUS BookInfoImpl::insert_book_info(const int &auto_book_id, const string &book_id,
                                          const string &book_name, const string &author_name,
                                          const string &bookType, const string &publishHouse,
                                          const string &publishTime, const string &bookIntro,
                                          const int &book_page, const int &languageType)
{
    cout << " t  h" << publishTime << " " << publishHouse << endl;
    return __base->insert_book_baseInfo(auto_book_id, book_id, book_name, author_name, bookType,
                                        publishHouse, publishTime, bookIntro, book_page, languageType);
}

SQL_STATUS BookInfoImpl::get_mostly_search_by_month_count(const string &monthTime,
                                                          vector<SearchStatisticsTable> &searchList, const int &count)
{
    return __search->get_mostly_search_by_month_count(monthTime, searchList, count);
}

SQL_STATUS BookInfoImpl::get_searchStatistics_info_by_id_and_daytime(const string &bookId,
                                                                     const string &dayTime, SearchStatisticsTable &stat)
{
    return __search->get_searchStatistics_info_by_id_and_daytime(bookId, dayTime, stat);
}

SQL_STATUS BookInfoImpl::insert_seacrh_inf(const SearchStatisticsTable &stat)
{
    return __search->insert_seacrh_inf(stat);
}

SQL_STATUS BookInfoImpl::update_seacrh_inf(const SearchStatisticsTable &stat)
{
    return __search->update_seacrh_inf(stat);
}

SQL_STATUS BookInfoImpl::plus_search_times(const int &auto_book_id, const string &book_id,
                                           const string &dayTime, const string &bookName)
{
    return __search->plus_search_times(auto_book_id, book_id, dayTime, bookName);
}

SQL_STATUS BookInfoImpl::get_all_ads(vector<BookADSTable> &res)
{
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_recommend_book(vector<CombineBook> &books,
                                            const int &curIndex)
{ // 获取个性化推荐书籍  用该函数占位

    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__
             << " conn is NULL "
             << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    auto allBook = conn->query<BookBaseInfoTable>();
    int sizeInt = 0;
    sizeInt = allBook.size() / 10;
    sizeInt += allBook.size() % 10 ? 1 : 0;
    int start = curIndex < sizeInt ? 10 * curIndex : (curIndex % sizeInt) * 10;
    int end = start + 10;
    vector<BookBaseInfoTable> base_info_books;
    for (; start < end; ++start)
    {
        if (start < allBook.size())
        {
            base_info_books.push_back(allBook[start]);
        }
    }
    //合法可进行下一步
    for (auto base_info : base_info_books)
    {
        BookScoreStat stat_info;
        SQL_STATUS ret = __grade->get_remark_count(stat_info.count, base_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            continue;
        //暂无评分
        if (stat_info.count == 0)
        {
            stat_info.avgScore = -1;
        }
        else
        {
            ret = __grade->get_average_score(stat_info.avgScore, base_info.autoBookId);
            if (ret != SQL_STATUS::EXE_sus)
                continue;
        }

        CombineBook buffer(std::move(base_info), std::move(stat_info));
        books.push_back(buffer);
    }

    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_books_by_fuzzy(vector<CombineBook> &books,
                                            const string &words)
{ //正则表达式匹配书籍 该函数存在问题 现已废弃
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__
             << " conn is NULL "
             << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string regexpInput = "\'[";
    for (const char &ch : words)
    {
        if (ch != ' ')
            regexpInput += (ch + ",");
    }
    regexpInput += "]\'";
    string cond = "select *  from BookBaseInfoTable "
                  "where bookId IN "
                  " (select bookId from "
                  " (select bookId,CONCAT(bookName,bookName,bookType) as description "
                  " from BookInfoTable "
                  " ) as concatTable "
                  " where  concatTable.description REGEXP " +
                  regexpInput + " ) ";

    auto base_info_books = conn->query<BookBaseInfoTable>(cond);
    cout << " cond is " << cond << " size is " << books.size() << endl;
    //合法可进行下一步
    for (auto base_info : base_info_books)
    {
        BookScoreStat stat_info;
        SQL_STATUS ret = __grade->get_remark_count(stat_info.count, base_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            continue;
        //暂无评分
        if (stat_info.count == 0)
        {
            stat_info.avgScore = -1;
        }
        else
        {
            ret = __grade->get_average_score(stat_info.avgScore, base_info.autoBookId);
            if (ret != SQL_STATUS::EXE_sus)
                continue;
        }

        CombineBook buffer(std::move(base_info), std::move(stat_info));
        books.push_back(buffer);
    }

    return SQL_STATUS::EXE_sus;
}

SQL_STATUS BookInfoImpl::get_browse_book(vector<CombineBook> &books,
                                         const int &curIndex)
{ // 浏览书城时随机推荐书籍
    return get_recommend_book(books, curIndex);
}

SQL_STATUS BookInfoImpl::get_book_offset(vector<CombineBook> &books,
                                         const int &offset, const int &count)
{
    vector<BookBaseInfoTable> base_info_books;
    SQL_STATUS ret = __base->get_book_baseInfo_by_offset(base_info_books, offset, count);
    if (ret == SQL_STATUS::EXE_err || ret == SQL_STATUS::Pool_err)
        return SQL_STATUS::EXE_err;
    else if (ret == SQL_STATUS::Illegal_info)
        return SQL_STATUS::Illegal_info;

    //合法可进行下一步
    for (auto base_info : base_info_books)
    {
        BookScoreStat stat_info;
        ret = __grade->get_remark_count(stat_info.count, base_info.autoBookId);
        if (ret != SQL_STATUS::EXE_sus)
            continue;
        //暂无评分
        if (stat_info.count == 0)
        {
            stat_info.avgScore = -1;
        }
        else
        {
            ret = __grade->get_average_score(stat_info.avgScore, base_info.autoBookId);
            if (ret != SQL_STATUS::EXE_sus)
                continue;
        }

        CombineBook buffer(std::move(base_info), std::move(stat_info));
        books.push_back(buffer);
    }

    return SQL_STATUS::EXE_sus;
}

#endif
