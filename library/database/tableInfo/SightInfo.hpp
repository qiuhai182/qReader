#pragma once

#ifndef _SIGHTINFO_H
#define _SIGHTINFO_H

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
     * 视线数据
     */
    struct SightTable
	{ // 视线信息表结构
		int userId;			    // 用户id
		string bookId;			// 书id
		int pageNum;			// 页数
		float x;				// x坐标
		float y;				// y坐标
		string timeStamp;       // 抓取时间
	};
	REFLECTION(SightTable,userId, bookId, pageNum,x, y, timeStamp);


    class SightInfo
    {
    public: 
        SightInfo(const bool create_status = true){
            __isCreate = create_status;
			if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create SightTable error ";
			}
        }
        /*
        * 客户端提交新视线数据
        * 写入数据库
        */
        SQL_STATUS insert_Sight(const SightTable &sight);

        //读取sightTable 模糊匹配一天的内的数据 year-month-day
        SQL_STATUS get_sight_by_timeStamp(vector<SightTable>& result,
                        const string &time_stamp,const int & user_id);

        SQL_STATUS get_a_day_data(const int &  user_id,const  string &  day_time,
                        vector<string> all_times); 

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
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@视线数据信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 ****************************************************************************/
SQL_STATUS SightInfo::create_table()
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

    conn->execute("DROP TABLE IF EXISTS SightTable");
    string state = 
    "CREATE TABLE SightTable( "
        " userId INTEGER NOT NULL, "
        " bookId TEXT NOT NULL,"
        " pageNum INTEGER NOT NULL , "
        " x FLOAT NOT NULL, "
        " y FLOAT NOT NULL , "
        " timeStamp TEXT NOT NULL "
    " ) ENGINE = InnoDB  DEFAULT CHARSET = UTF8MB4 " ;

    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}


SQL_STATUS SightInfo::insert_Sight(const SightTable &sight)
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
    int userId;			    // 用户id
		string bookId;			// 书id
		int pageNum;			// 页数
		float x;				// x坐标
		float y;				// y坐标
		string timeStamp;       // 抓取时间
    cout<<"  sight "<<sight.pageNum
    <<" us "<<sight.userId<<" boi "<<sight.bookId
    <<" x "<<sight.x<<" y "<<sight.y
    <<" timeStamp "<<sight.timeStamp   <<endl;
    if (conn->insert(sight) != 1)
    {
        cout <<endl
            << __FILE__ << " : " 
            << __LINE__ 
            << " SightTable insert error " << endl;
        return SQL_STATUS::EXE_err;   
    }
    cout << "视线数据写入SQL" << endl;
    return SQL_STATUS::EXE_sus;
}
//读取sightTable 模糊匹配一天的内的数据 year-month-day
SQL_STATUS SightInfo::get_sight_by_timeStamp(vector<SightTable>& result,
                        const string &time_stamp,const int & user_id)
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
	string cond = "where timeStamp LIKE \"\%" + time_stamp + "\%\" and userId =  " + to_string(user_id) ;
  
    result =  std::move(conn->query<SightTable>(cond));
	if (result.size() == 0)
		return SQL_STATUS::EXE_err;
    return SQL_STATUS::EXE_sus;
	
}

SQL_STATUS SightInfo::get_a_day_data(const int &  user_id,const  string &  day_time,
                        vector<string> all_times)
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ 
            << " conn is NULL "
            << " LINE  " << __LINE__ 
            << endl;
        return SQL_STATUS::Pool_err;
    }
    string state = "where timeStamp LIKE \"\%" + day_time + "\%\" and userId =  " + to_string(user_id) ;
    auto result = conn->query<SightTable>(state);
    for (auto &oneres : result)
    {
        // cout << "查询结果: " << oneres.timeStamp << endl;
        all_times.push_back(oneres.timeStamp);
    }
    return SQL_STATUS::EXE_sus ;
}


#endif
