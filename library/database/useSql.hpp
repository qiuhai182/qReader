#pragma once

#ifndef _USESQL_H
#define _USESQL_H

#include <iostream>
#include <thread>
#include <map>
#include <memory>
#include <typeinfo>
#include "tableInfo/UserInfo.hpp"
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
    
    class UserInfoImpl
    {
    public:
        UserInfoImpl()
        {
            __user = new UserInfo;
        };
        ~UserInfoImpl()
        {
            delete __user;
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
        UserInfo * __user;
    };

}



int UserInfoImpl::get_all_user(vector<UserInfoTable> &res)
{
    return __user->get_all_user(res);
}
SQL_STATUS UserInfoImpl::get_user_by_id(UserInfoTable &user, const int &user_id)
{
    return __user->get_user_by_id(user,user_id);
}
int UserInfoImpl::get_LikeUser_by_id(vector<UserInfoTable> &users, 
            option cond_option, string cond_value)
{
    return __user->get_LikeUser_by_id(users,cond_option,cond_value);
}
bool UserInfoImpl::judge_pwd(string userpwd)
{
    return __user->judge_pwd(userpwd);
}
SQL_STATUS UserInfoImpl::insert_user(const UserInfoTable &user)
{
    return __user->insert_user(user);
}
SQL_STATUS UserInfoImpl::insert_user(const int &usr_id, const string &user_nickname, 
                const int &is_update_head, const string &user_pwd, 
                const string &user_profile, const int &user_male, 
                const int &user_age,const int & is_Delete)
{
    return __user->insert_user(usr_id,user_nickname,
                                is_update_head,user_pwd,
                                user_profile,user_male,
                                user_age,is_Delete
                            );
}
SQL_STATUS UserInfoImpl::del_user(const int &user_id,const string & user_pwd)
{
    return __user->del_user(user_id,user_pwd);
}
SQL_STATUS UserInfoImpl::up_user(std::map<option,sqlUpdateVal> up_data)
{
    return __user->up_user(up_data);
}
//SQL_STATUS up_user(const UserInfoTable &user);
SQL_STATUS UserInfoImpl::update_password(const int &  user_id,const string &  user_pwd)
{
    return __user->update_password(user_id,user_pwd);
}
SQL_STATUS UserInfoImpl::get_max_account( int &  max_account)
{
    return __user->get_max_account(max_account);
}

        
#endif
