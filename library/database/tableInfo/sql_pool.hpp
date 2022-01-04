#pragma once

#ifndef _SQL_POOL_H
#define _SQL_POOL_H

#include <iostream>
#include <thread>
#include <memory>
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"
using namespace ormpp;
using namespace std;



namespace ormpp
{
	//执行返回
	enum class SQL_STATUS{
		Pool_err = 0, 
		Sql_err = 1,
		Illegal_info =2, 
		Create_err = 3,
		EXE_err = 4,
		EXE_sus = 5
	};
	//修改
	enum class COND_TYPE{
		String = 0,
		Int = 1
	};
	struct sqlUpdateVal{
		string strVal;
		int intVal;
		COND_TYPE type ; 
	};
		

	typedef  std::shared_ptr<dbng<mysql>>   pool_conn;
    pool_conn get_conn_from_pool()
	{ // 获取数据库连接池的一份连接
		ormpp_cfg cfg{};
		static bool isInitPool = false;
		auto &pool = connection_pool<dbng<mysql>>::instance();
		if (!isInitPool)
		{
			bool ret = config_manager::from_file(cfg, "ormpp.json"); // 根据ormpp.cfg文件内信息连接数据库
			if (!ret)
			{ // 文件信息连接数据库失败
				// 连接池数量 ip地址 用户 密码 数据库名 超时时间
				pool.init(10, "127.0.0.1", "myReader", "123456", "myDataBase", 10);
				return pool.get();
			}
			try
			{
				// 连接池数量 ip地址 用户 密码 数据库名 超时时间
				pool.init(cfg.db_conn_num, cfg.db_ip.data(), cfg.user_name.data(), cfg.pwd.data(), cfg.db_name.data(), cfg.timeout);
				isInitPool = true;
				cout << "num:" << cfg.db_conn_num << " ip: " << cfg.db_ip.data() << " user: " << cfg.user_name.data() << cfg.pwd.data() << endl;
			}
			catch (const std::exception &e)
			{
				std::cout << e.what() << std::endl;
				return NULL;
			}
			return pool.get();
		}
	}

	inline SQL_STATUS execute_sql(pool_conn & conn ,const string & cause,const string & state)
	{//执行原生语句
		int ret = conn->execute(state); 
		cout <<"cond is "<<state << " ret is " << ret<<endl ;
		cout<<" ret  is "<< ret << endl ;
		if (ret != 1)
		{
			cout<<endl
				<< cause << " error" << endl;
			return SQL_STATUS::EXE_err;
		}
		else{
			return SQL_STATUS::EXE_sus;
		}
	}
}


#endif


