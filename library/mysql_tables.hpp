#pragma once

#ifndef _MYSQL_TABLES_H
#define _MYSQL_TABLES_H

#include <iostream>
#include <thread>
#include <memory>
#include "ormpp/mysql.hpp"
#include "ormpp/dbng.hpp"
#include "ormpp/connection_pool.hpp"
#include "ormpp/ormpp_cfg.hpp"
#include "ormpp/entity.hpp"
// using namespace std::string_literals;
using namespace ormpp;
using namespace std;


// 定义项目的所有数据库表格、创建表格函数、获取数据库连接池连接函数


namespace ormpp
{


	struct UserInfoTable
	{ // 用户个人信息表结构
		string userId;			// 用户ID
		string userNickName;	// 用户昵称
		string userHeadImgUrl;	// 用户头像
		string userPwd;			// 用户密码
		string userProfile;		// 用户简介
		int userMale;			// 用户性别
		int userAge;			// 用户年龄
	};
	REFLECTION(UserInfoTable, userId, userNickName, userHeadImgUrl, userPwd, userProfile, userMale, userAge);


	struct UserReadCoredTable
	{ // 用户阅读信息表结构
		string userId;			// 用户ID
		string bookId;			// 书籍Id
		string startStamp;		// 开始时间
		string endStamp;		// 结束时间
		int pageNum;			// 页数
	};
	REFLECTION(UserReadCoredTable, userId, bookId, startStamp, endStamp, pageNum);


	struct BookInfoTable
	{ // 书籍信息表结构
		string bookId;			// 书籍id
		string bookName;		// 书籍name
		string bookHeadUrl;		// 封面url
		string bookDownUrl;		// 下载地址
		string authorName;		// 作者名
		string bookType;		// 类型
		int	   commentId;		// 首条评论id
	};
	REFLECTION(BookInfoTable, bookId, bookName, bookHeadUrl, bookDownUrl, authorName, bookType, commentId);


	struct UserShelfTable
	{ // 用户个人书架信息表结构
		string userId_bookId;	// 用户id+书籍id 作为额外标识，利于数据维护 
		string userId;			// 用户id
		string bookId;			// 书籍id
		int    currentPage;   	//新增字段，记录用户上次阅读位置，可省略
	};
	REFLECTION(UserShelfTable, userId_bookId, userId, bookId);


	struct HitsTable
	{ // 点赞信息表结构
		string hitId;			// 点赞id
		int hitNum;				// 点赞数
	};
	REFLECTION(HitsTable, hitId, hitNum);


	struct CommentTable
	{ // 评论信息表结构(使用对书籍、某一页和某个评论的评论)
		int commentId;			// 评论信息id
		string bookId;			// 书籍id
		int pageNum;			// 页数
		int parentId;			// 评论对象id(只有页数存在时有意义)
		string timeStamp;		// 时间
		string content;			// 内容
		string hitId;			// 点赞id
		string userId;			// 用户id
		string userName;		// 用户昵称
		string userHead;		// 用户头像
	};
    REFLECTION(CommentTable, commentId, bookId, pageNum, parentId,
                   timeStamp, content, hitId, userId, userName, userHead);

    struct NoteTable
	{ // 笔记信息表结构
		string userId;			// 用户id
		string noteId;			// 笔记id
		string hitId;			// 点赞id
		string commentId;		// 评论id
		string content;			// 内容
		string timeStamp;		// 时间
	};
	REFLECTION(NoteTable, userId, noteId, hitId, commentId, content, timeStamp);

	struct SightTable
	{ // 视线信息表结构
		string userId;			// 用户id
		string bookId;			// 书id
		int pageNum;			// 页数
		float x;				// x坐标
		float y;				// y坐标
		string timeStamp;       // 抓取时间
	};
	REFLECTION(SightTable, x, y, timeStamp, userId, bookId, pageNum);

	struct ReadSETable
	{ // 视线数据提交间隔记录
		string userId;			// 用户id
		string bookId;			// 书id
		int pageNum;			// 页数
		string startTime;		// 开始时间
		string endTime;			// 结束时间
		string dayTime;			// 数据记录日期，精确到天
	};
	REFLECTION(ReadSETable, userId, bookId, pageNum, startTime, endTime, dayTime);





	std::shared_ptr<dbng<mysql>> get_conn_from_pool()
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
				pool.init(10, "127.0.0.1", "iReader", "123456@iReader", "iReaderDataBase", 10);
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

	int create_user_table()
	{ //创建用户信息数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return -1;
		}
		conn->execute("DROP TABLE IF EXISTS UserInfoTable");
		ormpp_not_null not_null{{"userId"}};
		if(conn->create_datatable<UserInfoTable>(not_null))
			return 1;
		else
			return 0;
	}

	int create_shelf_table()
	{ //创建用户书架数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return -1;
		}
		conn->execute("DROP TABLE IF EXISTS UserShelfTable");
		ormpp_not_null not_null{{"userId","bookId"}};
		if(conn->create_datatable<UserShelfTable>(not_null))
			return 1;
		else
			return 0;
	}

	int create_readcored_table()
	{ //创建用户阅读记录数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return -1;
		}
		conn->execute("DROP TABLE IF EXISTS UserReadCoredTable");
		ormpp_not_null not_null{{"userId"}};
		if(conn->create_datatable<UserReadCoredTable>(not_null))
			return 1;
		else
			return 0;
	}

	int create_bookinfo_table()
	{ //创建书籍信息数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS BookInfoTable");
		ormpp_not_null not_null{{"bookId"}};
		if(conn->create_datatable<BookInfoTable>(not_null))
			return 1;
		else
			return 0;
	}

	int create_hits_table()
	{ //创建点赞信息数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS HitsTable");
		ormpp_not_null not_null{{"hitId"}};
		if(conn->create_datatable<HitsTable>(not_null))
			return 1;
		else
			return 0;
	}

	int create_comment_table()
	{ // 创建评论信息数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS CommentTable");
		// ormpp_not_null not_null{{"commentId"}};
		ormpp_key mainkey{"commentId"};
		if(conn->create_datatable<CommentTable>(mainkey))
			return 1;
		else
			return 0;
	}

	int create_note_table()
	{ // 创建笔记信息数据库表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS NoteTable");
		if(conn->create_datatable<NoteTable>())
			return 1;
		else
			return 0;
	}

	int create_sight_table()
	{ // 创建视线信息数据表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS SightTable");
		if(conn->create_datatable<SightTable>())
			return 1;
		else
			return 0;
	}

	int create_startEnd_table()
	{ // 创建单次阅读时长数据表
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn == NULL)
		{
			cout << "FILE: " << __FILE__ << " "
				 << "conn is NULL"
				 << " LINE  " << __LINE__ << endl;
			return 0;
		}
		conn->execute("DROP TABLE IF EXISTS ReadSETable");
		if(conn->create_datatable<ReadSETable>())
			return 1;
		else
			return 0;
	}

	int v()
	{ // 创建所有需要的数据库表格

		if (1 != create_user_table())
			LOG(INFO) << "初始化信息：创建数据库表格 UserInfoTable 失败";
		if (1 != create_shelf_table())
			LOG(INFO) << "初始化信息：创建数据库表格 UserShelfTable 失败";
		if (1 != create_readcored_table())
			LOG(INFO) << "初始化信息：创建数据库表格 UserReadCoredTable 失败";
		if (1 != create_bookinfo_table())
			LOG(INFO) << "初始化信息：创建数据库表格 BookInfoTable 失败";
		if (1 != create_hits_table())
			LOG(INFO) << "初始化信息：创建数据库表格 HitsTable 失败";
		if (1 != create_note_table())
			LOG(INFO) << "初始化信息：创建数据库表格 NoteTable 失败";
		if (1 != create_comment_table())
			LOG(INFO) << "初始化信息：创建数据库表格 CommentTable 失败";
		if (1 != create_sight_table())
			LOG(INFO) << "初始化信息：创建数据库表格 SightTable 失败";
		if (1 != create_startEnd_table())
			LOG(INFO) << "初始化信息：创建数据库表格 ReadSETable 失败";

		LOG(INFO) << "初始化信息：已创建所有数据库表格";
		return 1;
	}
}


#endif


