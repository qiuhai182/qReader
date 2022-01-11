<<<<<<< HEAD
#pragma once

#ifndef _USERINFO_H
#define _USERINFO_H

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
    static int MIN_ACCOUNT  = 10000 ;

    struct UserInfoTable
	{ // 用户个人信息表结构
		int userId;			// 用户ID
		string userNickName;	// 用户昵称
		int isUpdateHead;	    // 是否更改头像 1:true 0 false
		string userPwd;			// 用户密码
		string userProfile;		// 用户简介
		int userMale;			// 用户性别 0:male 1  -1：未添加 
		int userAge;			// 用户年龄 -1:未填
        int isDelete;           //是否注销 0:否 1：是
	};
	REFLECTION(UserInfoTable, userId, userNickName, isUpdateHead, userPwd, userProfile, userMale, userAge,isDelete);


    class UserInfo{
    public:
        UserInfo(const bool create_status = true)
        {
            __isCreate = create_status;
            if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create UserInfoTable error ";
			}
        }
        int get_all_user(vector<UserInfoTable> &res);
        SQL_STATUS get_user_by_id(UserInfoTable &user, const int &user_id);
        int get_LikeUser_by_id(vector<UserInfoTable> &users, 
                    option cond_option, string cond_value);
        bool judge_pwd(string userpwd);
        SQL_STATUS insert_user(const UserInfoTable &user);
        SQL_STATUS insert_user(const int &usr_id, const string &user_nickname, 
                        const int &is_update_head, const string &user_pwd, 
                        const string &user_profile, const int &user_male, 
                        const int &user_age,const int & is_Delete);
        SQL_STATUS del_user(const int &user_id,const string & user_pwd);
        SQL_STATUS up_user(std::map<option,sqlUpdateVal> up_data);
        //SQL_STATUS up_user(const UserInfoTable &user);
        SQL_STATUS update_password(const int &  user_id,const string &  user_pwd);
        SQL_STATUS get_max_account( int &  max_account);
    private:
        SQL_STATUS create_table();
        SQL_STATUS create_index();
        bool __isCreate;
    
    };

}


/******
 * calss : UserInfo
 */

SQL_STATUS UserInfo::get_max_account( int &  max_account)
{//获取可分配最大账号
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string state = "select *  from UserInfoTable  where userId = (select max(userId) from UserInfoTable) ";
    auto res = conn->query<UserInfoTable>(state);
    if(res.size() ==0 ){
        cout << "FILE: " << __FILE__ << " "
            << "get max account error"
            << " LINE  " << __LINE__ << endl;
        max_account = MIN_ACCOUNT;
        return SQL_STATUS::EXE_err;
    }
    max_account = res[0].userId;
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS UserInfo::create_index()
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state;
    state = "ALTER TABLE UserInfoTable ADD INDEX index_userId (userId)" ;
    return execute_sql(conn,"create userId indext",state);
}

SQL_STATUS UserInfo::create_table()
{
    /*创建个人信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;


    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }


    conn->execute("DROP TABLE IF EXISTS UserInfoTable");
    string state = 
    "CREATE TABLE UserInfoTable( "
        " userId INTEGER NOT NULL AUTO_INCREMENT , "
        " userNickName text , "
        " isupdateHead INTEGER DEFAULT 0 , "
        " userPwd text NOT NULL, "  
        " userProfile text, "       
        " userMale INTEGER  DEFAULT -1, "  
        " userAge INTEGER DEFAULT -1, "
        " isDelete INTEGER DEFAULT 0 , " 
        " PRIMARY KEY (userId) "
    " ) ENGINE = InnoDB AUTO_INCREMENT = " +to_string(MIN_ACCOUNT) + " DEFAULT CHARSET = UTF8MB4 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

int  UserInfo::get_all_user(vector<UserInfoTable> &res)
{
    //获取数据库所有用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return static_cast<int>(SQL_STATUS::Pool_err);
	}
	res = conn->query<UserInfoTable>();
	return res.size();
}

SQL_STATUS UserInfo::get_user_by_id(UserInfoTable &user, const int &user_id)
{//获取数据库所有用户信息

	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "where userId = " + std::to_string(user_id) ;
	auto res = conn->query<UserInfoTable>(state);
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    user =res[0];
	return SQL_STATUS::EXE_sus;
}

int UserInfo::get_LikeUser_by_id(vector<UserInfoTable> &users, 
                    string cond_option, string cond_value)
{//匹配数据库所有用户信息
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE:" << __FILE__ << ""
			 << "conn is NULL"
			 << __LINE__ << endl;
		return static_cast<int>(SQL_STATUS::Pool_err);
	}
	string cond = "where " + cond_option + " LIKE \'\%" + cond_value + "\%\'";
	auto res = conn->query<UserInfoTable>(cond);

	for (auto &i : res)
	{
		users.push_back(i);
	}
	return res.size();
}

bool UserInfo::judge_pwd(string userpwd)
{//判断注册密码是否符合规范

	const char *pwd = userpwd.c_str();
	int pwd_type[3] = {0};
	for (int i = 0; i < sizeof(pwd) / sizeof(char); ++i)
	{
		if (isdigit(pwd[i]) && !pwd_type[0])
			pwd_type[0] = 1;
		if (isalpha(pwd[i]) && !pwd_type[1])
			pwd_type[1] = 1;
		if (ispunct(pwd[i]) && !pwd_type[2])
			pwd_type[2] = 1;
	}
	if (pwd_type[0] && pwd_type[1] && pwd_type[2]) //包含字母、数字、标点符号或特殊字符
		return true;
	else
		return false;
}

SQL_STATUS UserInfo::insert_user(const UserInfoTable &user)
{//结构体序列化

    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

    if(user.userId != 0){
        cout << "非法账号注册 "<<user.userId <<endl;
        return SQL_STATUS::Illegal_info;
    }else{ //注册用户
		// if (-1 == judge_pwd(user.userPwd))
		// { // 密码格式不符合要求
		// 	return -4;
		// }
		if (conn->insert(user) != 1)
		{
			cout <<endl
                << __FILE__ << " : " << __LINE__ << "  insert error" << endl;
			return SQL_STATUS::EXE_err;
		}
		else
			return SQL_STATUS::EXE_sus;
	}
}

SQL_STATUS UserInfo::insert_user(const int &usr_id, const string &user_nickname, 
                        const int &is_update_head, const string &user_pwd, 
                        const string &user_profile, const int &user_male, 
                        const int &user_age,const int & is_Delete)
{
    UserInfoTable user;
    user.userId = usr_id;
    user.userNickName = user_nickname;
    user.isUpdateHead = is_update_head;
    user.userPwd = user_pwd;
    user.userProfile = user_profile;
    user.userAge = user_age;
    user.userMale = user_male;
    user.isDelete =  is_Delete;

    return this->insert_user(user);

}

SQL_STATUS UserInfo::del_user(const int &user_id,const string & user_pwd)
{//数据库删除用户
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return  SQL_STATUS::Pool_err;
	}
	
    string state,cond;
    state += "update UserInfoTable set ";
    state += " isDelete = " + std::to_string(1);
    state += " where  userId = " + std::to_string(user_id) 
            + " and userPwd = \'" + user_pwd + "\'" ;

    return execute_sql(conn,"delete",state);
}

SQL_STATUS UserInfo::up_user(std::map<option,sqlUpdateVal> up_data)
{//更改用户信息
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}else{
        if(up_data.size() == 0)
            return SQL_STATUS::Illegal_info;
    }

    string state ,buffer,cond;
    state = "update UserInfoTable set ";

    for(auto up : up_data)
    {
        if(up.first == "userId"){
            cond = std::to_string(up.second.intVal);
            continue;
        }
        if( up.second.type == COND_TYPE::Int )
        {
            buffer = " " + up.first + " =  " +  std::to_string(up.second.intVal) + "  ,";
        }else{
            buffer = " " + up.first + " =  \'" +  up.second.strVal  + "\' ,";
        }
        state += buffer;
    }
    //去除多余的,
    state.replace(state.rfind(","), 1, "");
    //添加条件
    state += " where userId = " + cond ;
    
    return execute_sql(conn,"update userInfo ",state);
}

SQL_STATUS UserInfo::update_password(const int &  user_id,const string &  user_pwd)
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return  SQL_STATUS::Pool_err;
	}
	
    string state,cond;
    state += "update UserInfoTable set ";
    state += " userPwd = \'" + user_pwd + "\' ";
    state += " where  userId = " + std::to_string(user_id);

    return execute_sql(conn,"delete",state);
}

#endif
=======
#pragma once

#ifndef _USERINFO_H
#define _USERINFO_H

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
    static int MIN_ACCOUNT  = 10000 ;

    struct UserInfoTable
	{ // 用户个人信息表结构
		int userId;			// 用户ID
		string userNickName;	// 用户昵称
		int isUpdateHead;	    // 是否更改头像 1:true 0 false
		string userPwd;			// 用户密码
		string userProfile;		// 用户简介
		int userMale;			// 用户性别 0:male 1  -1：未添加 
		int userAge;			// 用户年龄 -1:未填
        int isDelete;           //是否注销 0:否 1：是
	};
	REFLECTION(UserInfoTable, userId, userNickName, isUpdateHead, userPwd, userProfile, userMale, userAge,isDelete);


    class UserInfo{
    public:
        UserInfo(const bool create_status = true)
        {
            __isCreate = create_status;
            if(!__isCreate){
				if(SQL_STATUS::EXE_sus != this->create_table())
					throw "create UserInfoTable error ";
			}
        }
        int get_all_user(vector<UserInfoTable> &res);
        SQL_STATUS get_user_by_id(UserInfoTable &user, const int &user_id);
        int get_LikeUser_by_id(vector<UserInfoTable> &users, 
                    option cond_option, string cond_value);
        bool judge_pwd(string userpwd);
        SQL_STATUS insert_user(const UserInfoTable &user);
        SQL_STATUS insert_user(const int &usr_id, const string &user_nickname, 
                        const int &is_update_head, const string &user_pwd, 
                        const string &user_profile, const int &user_male, 
                        const int &user_age,const int & is_Delete);
        SQL_STATUS del_user(const int &user_id,const string & user_pwd);
        SQL_STATUS up_user(std::map<option,sqlUpdateVal> up_data);
        //SQL_STATUS up_user(const UserInfoTable &user);
        SQL_STATUS update_password(const int &  user_id,const string &  user_pwd);
        SQL_STATUS get_max_account( int &  max_account);
    private:
        SQL_STATUS create_table();
        SQL_STATUS create_index();
        bool __isCreate;
    
    };



/******
 * calss : UserInfo
 */

SQL_STATUS UserInfo::get_max_account( int &  max_account)
{//获取可分配最大账号
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }

    string state = "select *  from UserInfoTable  where userId = (select max(userId) from UserInfoTable) ";
    auto res = conn->query<UserInfoTable>(state);
    if(res.size() ==0 ){
        cout << "FILE: " << __FILE__ << " "
            << "get max account error"
            << " LINE  " << __LINE__ << endl;
        max_account = MIN_ACCOUNT;
        return SQL_STATUS::EXE_err;
    }
    max_account = res[0].userId;
    return SQL_STATUS::EXE_sus;
}

SQL_STATUS UserInfo::create_index()
{
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }
    string state;
    state = "ALTER TABLE UserInfoTable ADD INDEX index_userId (userId)" ;
    return execute_sql(conn,"create userId indext",state);
}

SQL_STATUS UserInfo::create_table()
{
    /*创建个人信息表 */
    if(this->__isCreate == true)
        return SQL_STATUS::Create_err;


    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
    if (conn == NULL)
    {
        cout << "FILE: " << __FILE__ << " "
            << "conn is NULL"
            << " LINE  " << __LINE__ << endl;
        return SQL_STATUS::Pool_err;
    }


    conn->execute("DROP TABLE IF EXISTS UserInfoTable");
    string state = 
    "CREATE TABLE UserInfoTable( "
        " userId INTEGER NOT NULL AUTO_INCREMENT , "
        " userNickName text , "
        " isupdateHead INTEGER DEFAULT 0 , "
        " userPwd text NOT NULL, "  
        " userProfile text, "       
        " userMale INTEGER  DEFAULT -1, "  
        " userAge INTEGER DEFAULT -1, "
        " isDelete INTEGER DEFAULT 0 , " 
        " PRIMARY KEY (userId) "
    " ) ENGINE = InnoDB AUTO_INCREMENT = " +to_string(MIN_ACCOUNT) + " DEFAULT CHARSET = UTF8MB4 " ;


    if(conn->execute(state)){
        //设置不可创建
        this->__isCreate =true;
        return SQL_STATUS::EXE_sus;
    }else
        return SQL_STATUS::EXE_err;
}

int  UserInfo::get_all_user(vector<UserInfoTable> &res)
{
    //获取数据库所有用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return static_cast<int>(SQL_STATUS::Pool_err);
	}
	res = conn->query<UserInfoTable>();
	return res.size();
}

SQL_STATUS UserInfo::get_user_by_id(UserInfoTable &user, const int &user_id)
{//获取数据库所有用户信息

	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}
	string state = "where userId = " + std::to_string(user_id) ;
	auto res = conn->query<UserInfoTable>(state);
    if(res.size() == 0){
        return SQL_STATUS::EXE_err;
    }
    user =res[0];
	return SQL_STATUS::EXE_sus;
}

int UserInfo::get_LikeUser_by_id(vector<UserInfoTable> &users, 
                    string cond_option, string cond_value)
{//匹配数据库所有用户信息
	
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE:" << __FILE__ << ""
			 << "conn is NULL"
			 << __LINE__ << endl;
		return static_cast<int>(SQL_STATUS::Pool_err);
	}
	string cond = "where " + cond_option + " LIKE \'\%" + cond_value + "\%\'";
	auto res = conn->query<UserInfoTable>(cond);

	for (auto &i : res)
	{
		users.push_back(i);
	}
	return res.size();
}

bool UserInfo::judge_pwd(string userpwd)
{//判断注册密码是否符合规范

	const char *pwd = userpwd.c_str();
	int pwd_type[3] = {0};
	for (int i = 0; i < sizeof(pwd) / sizeof(char); ++i)
	{
		if (isdigit(pwd[i]) && !pwd_type[0])
			pwd_type[0] = 1;
		if (isalpha(pwd[i]) && !pwd_type[1])
			pwd_type[1] = 1;
		if (ispunct(pwd[i]) && !pwd_type[2])
			pwd_type[2] = 1;
	}
	if (pwd_type[0] && pwd_type[1] && pwd_type[2]) //包含字母、数字、标点符号或特殊字符
		return true;
	else
		return false;
}

SQL_STATUS UserInfo::insert_user(const UserInfoTable &user)
{//结构体序列化

    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}

    if(user.userId != 0){
        cout << "非法账号注册 "<<user.userId <<endl;
        return SQL_STATUS::Illegal_info;
    }else{ //注册用户
		// if (-1 == judge_pwd(user.userPwd))
		// { // 密码格式不符合要求
		// 	return -4;
		// }
		if (conn->insert(user) != 1)
		{
			cout <<endl
                << __FILE__ << " : " << __LINE__ << "  insert error" << endl;
			return SQL_STATUS::EXE_err;
		}
		else
			return SQL_STATUS::EXE_sus;
	}
}

SQL_STATUS UserInfo::insert_user(const int &usr_id, const string &user_nickname, 
                        const int &is_update_head, const string &user_pwd, 
                        const string &user_profile, const int &user_male, 
                        const int &user_age,const int & is_Delete)
{
    UserInfoTable user;
    user.userId = usr_id;
    user.userNickName = user_nickname;
    user.isUpdateHead = is_update_head;
    user.userPwd = user_pwd;
    user.userProfile = user_profile;
    user.userAge = user_age;
    user.userMale = user_male;
    user.isDelete =  is_Delete;

    return this->insert_user(user);

}

SQL_STATUS UserInfo::del_user(const int &user_id,const string & user_pwd)
{//数据库删除用户
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return  SQL_STATUS::Pool_err;
	}
	
    string state,cond;
    state += "update UserInfoTable set ";
    state += " isDelete = " + std::to_string(1);
    state += " where  userId = " + std::to_string(user_id) 
            + " and userPwd = \'" + user_pwd + "\'" ;

    return execute_sql(conn,"delete",state);
}

SQL_STATUS UserInfo::up_user(std::map<option,sqlUpdateVal> up_data)
{//更改用户信息
    auto conn = get_conn_from_pool();
    conn_guard guard(conn);
	if (conn == NULL)
	{
		LOG(WARNING) << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return SQL_STATUS::Pool_err;
	}else{
        if(up_data.size() == 0)
            return SQL_STATUS::Illegal_info;
    }

    string state ,buffer,cond;
    state = "update UserInfoTable set ";

    for(auto up : up_data)
    {
        if(up.first == "userId"){
            cond = std::to_string(up.second.intVal);
            continue;
        }
        if( up.second.type == COND_TYPE::Int )
        {
            buffer = " " + up.first + " =  " +  std::to_string(up.second.intVal) + "  ,";
        }else{
            buffer = " " + up.first + " =  \'" +  up.second.strVal  + "\' ,";
        }
        state += buffer;
    }
    //去除多余的,
    state.replace(state.rfind(","), 1, "");
    //添加条件
    state += " where userId = " + cond ;
    
    return execute_sql(conn,"update userInfo ",state);
}

SQL_STATUS UserInfo::update_password(const int &  user_id,const string &  user_pwd)
{
    auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return  SQL_STATUS::Pool_err;
	}
	
    string state,cond;
    state += "update UserInfoTable set ";
    state += " userPwd = \'" + user_pwd + "\' ";
    state += " where  userId = " + std::to_string(user_id);

    return execute_sql(conn,"delete",state);
}



}



#endif
>>>>>>> 6e3698d0b0c8dda16fd56ca04329a5f75e4f0595
