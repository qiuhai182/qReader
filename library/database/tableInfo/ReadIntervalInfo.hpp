#pragma once

#ifndef _READINTERVALINFO_H
#define _READINTERVALINFO_H

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
     * 提交间隔数据
     */
    struct ReadIntervalTable
	{ // 视线数据提交间隔记录
		int userId;			    // 用户id
		string bookId;			// 书id
		int pageNum;			// 页数
		string startTime;		// 开始时间
		string endTime;			// 结束时间
		string dayTime;			// 数据记录日期，精确到天
	};
	REFLECTION(ReadIntervalTable, userId, bookId, pageNum, startTime, endTime, dayTime);

    class ReadIntervalInfo
    {
    public: 
        ReadIntervalInfo(const bool create_status = true){
            __isCreate = create_status;
			if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create ReadIntervalTable error ";
			}
        }
        /*
        * 客户端提交一批次数据的开始时间、结束时间、记录时间
        * 写入数据库
        */
        SQL_STATUS insert_internal_info(const ReadIntervalTable &read_interval);
        
        // SQL_STATUS up_book();
        // SQL_STATUS up_book();
		
    private:
		SQL_STATUS create_table();
        // SQL_STATUS create_times_index();
        // SQL_STATUS create_indexs();
        bool __isCreate;
    };

}


/****************************************************************************
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@提交视线信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS ReadIntervalInfo::create_table()
{
    /*创建视线信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;

    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    conn->execute("DROP TABLE IF EXISTS ReadIntervalTable");
    string state = 
    "CREATE TABLE ReadIntervalTable( "
        " userId INTEGER NOT NULL, "
        " bookId TEXT NOT NULL,"
        " pageNum INTEGER NOT NULL , "
        " startTime TEXT NOT NULL, "
        " endTime TEXT NOT NULL , "
        " dayTime TEXT NOT NULL "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

SQL_STATUS ReadIntervalInfo::insert_internal_info(const ReadIntervalTable &read_interval)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    if (conn->insert(read_interval) != 1)
    {
        cout << __FILE__ << " : " 
        << __LINE__ << "ReadIntervalTable insert error" 
        << endl;
        return SQL_STATUS::EXE_err;
    }

    return SQL_STATUS::EXE_sus;
}

#endif
