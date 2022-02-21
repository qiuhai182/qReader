#pragma once

#ifndef _SIGHTSQL_H
#define _SIGHTSQL_H

#include <iostream>
#include <thread>
#include <map>
#include <memory>
#include <typeinfo>
#include "tableInfo/ReadIntervalInfo.hpp"
#include "tableInfo/SightInfo.hpp"
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
    
    class SightInfoImpl
    {
    public:
        SightInfoImpl()
        {
            __sight = new SightInfo;
            __interval = new ReadIntervalInfo;
        };
        ~SightInfoImpl()
        {
            delete __sight;
            delete __interval;
        }
        SQL_STATUS insert_Sight(const SightTable &sight);

        //读取sightTable 模糊匹配一天的内的数据 year-month-day
        SQL_STATUS get_sight_by_timeStamp(vector<SightTable>& result,
                        const string &time_stamp,const int & user_id);

        SQL_STATUS insert_internal_info(const ReadIntervalTable &read_interval);

        SQL_STATUS get_a_day_data(const int &  use_id,const  string &  day_time,
                        vector<string> all_times);

    private:
        SightInfo * __sight;
        ReadIntervalInfo * __interval;
    };

}

SQL_STATUS SightInfoImpl::insert_Sight(const SightTable &sight)
{
    return __sight->insert_Sight(sight);
}

SQL_STATUS SightInfoImpl::get_sight_by_timeStamp(vector<SightTable>& result,
                const string &time_stamp,const int & user_id)
{
    return __sight->get_sight_by_timeStamp(result,time_stamp,user_id);
}

SQL_STATUS SightInfoImpl::insert_internal_info(const ReadIntervalTable &read_interval)
{
    return __interval->insert_internal_info(read_interval);
}

SQL_STATUS SightInfoImpl::get_a_day_data(const int &  use_id,const  string &  day_time,
                        vector<string> all_times)
{
    return __sight->get_a_day_data(use_id,day_time,all_times);
}
        
#endif
