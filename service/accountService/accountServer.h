#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "database/useSql.hpp"
#include "public/location.hpp"
#include "public/service.hpp"
#include "bitmap.hpp"
#include "account.pb.h"
#include "common.pb.h"
using namespace std;
using namespace service;
using namespace ormpp;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
//DEFINE_string(ip, "192.168.91.128", "用于文件下载的ip外网地址");
DEFINE_int32(accountPort, 8007, "服务端口");
DEFINE_string(stringPort, "8007", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_string(imageIPort, FLAGS_ip + ":8006" , "下载图片ip:port");
DEFINE_string(headUrlPre, "http://" + FLAGS_imageIPort + "/fileService/fileDownFun/images/", "头像url前缀");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小



namespace accountService
{

	BitMap bitmap;

	class accountServiceImpl : public accountService
	{ // 登录服务
	private:
		ormpp::UserInfoImpl __userInfoSql;
		int __current_max_account;
	public:
		accountServiceImpl():__userInfoSql()
		{
			__current_max_account = -1 ;
			__userInfoSql.get_max_account(__current_max_account);
			cout<<" max is "<<__current_max_account<<endl ;
			if( -1 == __current_max_account)
			{
				LOG(WARNING) << "获取当前最大账号失败"<<endl ;
				throw "start user service failed" ;
			}	
			
		};
		virtual ~accountServiceImpl(){};
		virtual void loginFun(google::protobuf::RpcController *control_base,
							  const loginInfoReq *request,
							  commonService::commonResp *response,
							  google::protobuf::Closure *done)
		{ // 登录函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "], 客户端ip+port: " << control->remote_side()
					  << ", 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			UserInfoTable bufUser;
			SQL_STATUS ret = __userInfoSql.get_user_by_id(bufUser, request->userid());
			if (SQL_STATUS::EXE_sus != ret  || bufUser.isUpdateHead == 1)
			{ // 请求登录的账户不存在
				LOG(INFO) << endl
						  << control->remote_side() << " 请求登录的账号: " << request->userid() << " 不存在" << endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Useless_inf));
				response->set_errorres("账户不存在");
				return;
			}

			if (bufUser.userPwd != request->userpwd() || bufUser.userId != request->userid())
			{ // 账号或密码有误，从数据库匹配
				LOG(INFO) << endl
						  << control->remote_side() << " 请求登录账号: " << request->userid() << " 失败,密码错误" << endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Useless_inf));
				response->set_errorres("密码错误");
			}
			else
			{
				if (bitmap.HasExisted(request->userid()))
				{ // 请求保持账号登陆状态
					LOG(INFO) << endl
							  << control->remote_side() << " 请求保持账号: " << request->userid() << " 登陆状态，成功" << endl;
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				}
				else
				{ // 账号、密码准确无误
					bitmap.Set(request->userid());
					LOG(INFO) << endl
							  << control->remote_side() << " 请求登陆账号: " << request->userid() << " 成功" << endl;
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				}
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void userInfoFun(google::protobuf::RpcController *control_base,
								 const userInfoReq *request,
								 userInfoResp *response,
								 google::protobuf::Closure *done)
		{ // 获取用户数据函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			UserInfoTable bufUser;
			if (ormpp::SQL_STATUS::EXE_sus == __userInfoSql.get_user_by_id(bufUser, request->userid()) )
			{ // 获取数据库数据
				LOG(INFO) << endl
						  << control->remote_side() << " 请求获取账号: " << request->userid() << " 的数据,成功" << endl;
				response->set_userid(bufUser.userId);
				response->set_nickname(bufUser.userNickName);
				response->set_profile(bufUser.userProfile);
				//头像
				string headPath ;
				if(bufUser.isUpdateHead == 0)
				{
					headPath = FLAGS_headUrlPre + "default.png";
					response->set_headimgurl(headPath);
				}
				else
				{
					headPath = FLAGS_headUrlPre + to_string(bufUser.userId) + "_head.png";
					response->set_headimgurl(headPath);
				}
				if(bufUser.userMale != -1)
					response->set_male(bufUser.userMale);
				if(bufUser.userAge != -1)
					response->set_age(bufUser.userAge);
				
			}
			else
			{ // 获取用户数据失败
				LOG(INFO) << endl
						  << control->remote_side() << " 请求获取账号: " << request->userid() << " 的数据,失败" << endl;
				response->set_userid(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void registerFun(google::protobuf::RpcController *control_base,
								 const registerReq *request,
								 registerResp *response,
								 google::protobuf::Closure *done)
		{ // 注册用户函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 用户注册密码 : " << ": " << request->userpwd()
					  << " (attached : " << control->request_attachment() << ")";
			
			UserInfoTable newUser = {
				0,"书友",0,
				request->userpwd(),"你好 这个人很懒~",
				-1,-1,0};
			ormpp::SQL_STATUS ret = __userInfoSql.insert_user(newUser);
			if(ormpp::SQL_STATUS::EXE_sus ==  ret)
			{
				__current_max_account++ ;
				// 注册并录入数据库成功
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: " <<__current_max_account << " 成功" << endl;
				response->set_userid(__current_max_account);
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus)); // 设置返回值
			}
			else
			{
				// 注册失败
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: "  << " 失败,原因:"
						  <<static_cast<int>(ret) << endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err)); // 设置返回值
			}
			
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void updatePasswdFun(::google::protobuf::RpcController* control_base,
                       const ::accountService::updateReq* request,
                       ::commonService::commonResp* response,
                       ::google::protobuf::Closure* done)
		{//修改密码服务
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			
			ormpp::SQL_STATUS ret = __userInfoSql.update_password(request->userid(),request->newpassword()) ;
			if(ret == ormpp::SQL_STATUS::EXE_sus )
			{
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				LOG(INFO) << endl
						  << control->remote_side() << "  修改密码成功" << endl;
			}else{
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				LOG(INFO) << endl
						  << control->remote_side() << "  修改密码失败" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}

		}

		virtual void getAllUserInfoFun(google::protobuf::RpcController *control_base,
									   const getAllUserInfoReq *request,
									   userInfoRespList *response,
									   google::protobuf::Closure *done)
		{ // 获取所有用户信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			vector<UserInfoTable> res;
			__userInfoSql.get_all_user(res); // 获得所有用户信息
			int count = 0;
			for (auto u : res)
			{ // 把所有用户信息回发用户
				++count;
				auto user = response->add_lists();
				user->set_userid(u.userId);
				user->set_nickname(u.userNickName);
				user->set_profile(u.userProfile);
				string headPath ;
				if(u.isUpdateHead == 0)
				{
					headPath = FLAGS_headUrlPre + "default.png";
					user->set_headimgurl(headPath);
				}
				else
				{
					headPath = FLAGS_headUrlPre + to_string(u.userId) + "_head.png";
					user->set_headimgurl(headPath);
				}
				if(u.userMale != -1)
					user->set_male(u.userMale);
				if(u.userAge != -1)
					user->set_age(u.userAge);
			}
			response->set_count(count);
			LOG(INFO) << endl
					  << control->remote_side() << " 请求获取所有用户数据,成功" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getLikeUserInfoFun(google::protobuf::RpcController *control_base,
										const getLikeUserInfoReq *request,
										userInfoRespList *response,
										google::protobuf::Closure *done)
		{ // 获取匹配用户信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			vector<UserInfoTable> res;
			__userInfoSql.get_LikeUser_by_id(res, request->cond_option(), request->cond_value()); // 获得匹配用户信息
			int count = 0;
			if (!res.empty())
			{ // 匹配到对应用户信息
				for (auto u : res)
				{ // 把所有用户信息回发用户
					++count;
				auto user = response->add_lists();
				user->set_userid(u.userId);
				user->set_nickname(u.userNickName);
				string headPath ;
				if(u.isUpdateHead == 0)
				{
					headPath = FLAGS_headUrlPre + "default.png";
					user->set_headimgurl(headPath);
				}
				else
				{
					headPath = FLAGS_headUrlPre + to_string(u.userId) + "_head.png";
					user->set_headimgurl(headPath);
				}
				user->set_profile(u.userProfile);
				if(u.userMale != -1)
					user->set_male(u.userMale);
				if(u.userAge != -1)
					user->set_age(u.userAge);
				}
				response->set_count(count); // 设置回发数量
				LOG(INFO) << endl
						  << control->remote_side() << " 请求获取匹配值的用户数据,成功" << endl;
			}
			else
			{
				response->set_count(count); // 设置回发数量
				LOG(INFO) << endl
						  << control->remote_side() << "请求获取匹配值的用户数据,失败" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void setUserInfoFun(google::protobuf::RpcController *control_base,
									const userInfoResp *request,
									commonService::commonResp *response,
									google::protobuf::Closure *done)
		{ // 设置用户数据函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			if (request->has_userid())
			{ 
				UserInfoTable user ;
				ormpp::SQL_STATUS ret = __userInfoSql.get_user_by_id(user, request->userid());
				if( ormpp::SQL_STATUS::EXE_sus == ret )
				{
					ormpp::sqlUpdateVal buffer;
					map<string,sqlUpdateVal>update;

					buffer.intVal = request->userid();
					buffer.type = ormpp::COND_TYPE::Int;
					update["userId"] = buffer;
					if( request->has_nickname())
					{
						buffer.strVal = request->nickname();
						buffer.type = ormpp::COND_TYPE::String;
						update["userNickName"] = buffer;
					}
					if( request->has_profile())
					{
						buffer.strVal = request->profile();
						buffer.type = ormpp::COND_TYPE::String;
						update["userProfile"] = buffer;
					}
					if( request->has_male())
					{
						buffer.intVal = request->male();
						buffer.type = ormpp::COND_TYPE::Int;
						update["userMale"] = buffer;
					}
					if( request->has_age())
					{
						buffer.intVal = request->age();;
						buffer.type = ormpp::COND_TYPE::Int;
						update["userAge"] = buffer;
					}
					if( request->has_headimgdata())
					{
						//前端默认格式
						buffer.intVal = 1;
						buffer.type = ormpp::COND_TYPE::Int;
						update["isUpdateHead"] =buffer;
						//存入
						string suffix = "png" ;
						string headPath = FLAGS_imagesPath+ to_string(request->userid()) + "_head." + suffix;
						std::ofstream outFile(headPath, std::ios::binary | std::ios::trunc);
						outFile.write(&request->headimgdata()[0], request->headimgdata().size()); 
					}

					if( ormpp::SQL_STATUS::EXE_sus == __userInfoSql.up_user(update) )
					{
						response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
						response->set_errorres("修改成功");
						LOG(INFO) << endl
								<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息,修改成功" << endl;
					}else{
						response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
						response->set_errorres("数据库错误");
						LOG(INFO) << endl
								<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息失败" << endl;	
					}

				}else{
					response->set_code(static_cast<int>(RES_CODE::FAL));
					response->set_errorres("账号不存在");
					LOG(INFO) << endl
				 			  << control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息,账号不存在" << endl;
				}
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void delUserFun(google::protobuf::RpcController *control_base,
								const loginInfoReq *request,
								commonService::commonResp *response,
								google::protobuf::Closure *done)
		{ // 删除用户
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			if (request->has_userid())
			{
				ormpp::SQL_STATUS ret = __userInfoSql.del_user(request->userid(),request->userpwd());
				if ( ret == ormpp::SQL_STATUS::EXE_sus)
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
					LOG(INFO) << endl
							  << control->remote_side() << "请求注销用户账号: " << request->userid() << " 成功" << endl;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
					response->set_errorres("注销失败");
					LOG(INFO) << endl
							  << control->remote_side() << "请求注销用户账号: " << request->userid() << " 失败" << endl;
				}
			}	
		}
	};

}

