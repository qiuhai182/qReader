#pragma once

#include <iostream>
#include <vector>
#include "mysql_tables.hpp"
using namespace std;
using namespace ormpp;


// 封装账户信息数据库操作函数


int get_all_user(vector<UserInfoTable> &res)
{ //获取数据库所有用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	res = conn->query<UserInfoTable>();
	return res.size();
}


int get_user_by_id(UserInfoTable &user, const string &user_id)
{ //通过指定id获取数据库用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where userId=\"" + user_id + "\"";
	auto res = conn->query<UserInfoTable>(cond);
	if (res.size() == 0)
		return -1;
	else
	{
		user = res[0];
		return 1;
	}
	return -1;
}


int get_LikeUser_by_id(vector<UserInfoTable> &users, string cond_option, string cond_value)
{ //匹配数据库所有用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE:" << __FILE__ << ""
			 << "conn is NULL"
			 << __LINE__ << endl;
		return -1;
	}
	string cond = "where " + cond_option + " LIKE \'\%" + cond_value + "\%\'";
	auto res = conn->query<UserInfoTable>(cond);
	if (res.size() == 0)
		return -1;
	for (auto &i : res)
	{
		users.push_back(i);
	}
	return res.size();
}


int judge_pwd(string userpwd)
{ //判断注册密码是否符合规范
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
		return 1;
	else
		return -1;
}


int insert_user(const UserInfoTable &user)
{ // 数据库新增用户信息	 -4：密码不符要求    -3：账户不存在    -2：账户已存在    -1：连接数据库出错    0：失败    1：成功
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}

	string uid = user.userId;
	UserInfoTable bufUser;
	if (1 == get_user_by_id(bufUser, uid))
		return -2; //已存在该账户
	else
	{ //注册用户
		// if (-1 == judge_pwd(user.userPwd))
		// { // 密码格式不符合要求
		// 	return -4;
		// }
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		if (conn->insert(user) != 1)
		{
			cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
			return 0;
		}
		else
			return 1;
	}
	return -1;
}


int insert_user(const string &usr_id, const string &user_nickname, const string &user_head_imgurl, const string &user_pwd, const string &user_profile, const int &user_male, const int &user_age)
{ //数据库新增用户信息
	UserInfoTable user{usr_id, user_nickname, user_head_imgurl, user_pwd, user_profile, user_male, user_age};
	return insert_user(user);
}


int del_user(const string &user_id)
{ //数据库删除用户		-4：密码不符要求    -3：账号不存在    -2：账户已存在    -1：连接数据库出错    0：失败    1：成功
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	string cond = "where userId = \'" + user_id + "\';
	auto res = conn->query<UserInfoTable>(cond);
	if (res.size() == 0)
		return -3; //账号不存在
	else
	{
		auto conn = get_conn_from_pool();
		conn_guard guard(conn);
		string cond = "userId = \'" + user_id +"\'";
		if (conn->delete_records<UserInfoTable>(cond))
			return 1;
		else
			return 0;
	}
}


int up_user(const UserInfoTable &user)
{ //由另一个重载函数调用，数据库更新用户信息
	auto conn = get_conn_from_pool();
	conn_guard guard(conn);
	if (conn == NULL)
	{
		cout << "FILE: " << __FILE__ << " "
			 << "conn is NULL"
			 << " LINE  " << __LINE__ << endl;
		return -1;
	}
	//无主键
	del_user(user.userId);
	if (conn->update(user, "userId") != 1)
	{
		cout << __FILE__ << " : " << __LINE__ << "insert error" << endl;
		return -1;
	}
	else
		return 1;
}


int up_user(const string &usr_id, const string &user_nickname, const string &user_head_imgurl, const string &user_profile, const string &user_pwd, const int &user_male, const int &user_age)
{ //由server.h调用，删除原有user,重新添加		-4：密码不符要求    -3：账号不存在    -2：账户已存在    -1：连接数据库出错    0：失败    1：成功
	
	// UserInfoTable user ;
	// //获取信息在确定部分覆盖
	// int ret = get_user_by_id(user,usr_id);

	
	int int_res = del_user(usr_id);
	if (int_res == 1)
	{
		UserInfoTable user{usr_id, user_nickname, user_head_imgurl, user_profile, user_pwd, user_male, user_age};
		return up_user(user);
	}
	else
		return int_res;
}


int  update_password(string usr_id,string usr_passwd)
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

	
	string cond = "update  UserInfoTable set userPwd = \'" 
				+ usr_passwd + "\' where userId = \'" + usr_id  + "\'";
	
	int ret =  conn->execute(cond);
	cout << "ret  is "<<ret<<" "<<cond<<endl ;
	if(ret)
	{
		return ret ;	
	}else{
		return -1 ;
	}

}

