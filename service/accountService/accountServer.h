#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "userdata.hpp"
#include "bookdata.hpp"
#include "usershelfdata.hpp"
#include "commentdata.hpp"
#include "bitmap.hpp"
#include "sightdata.hpp"
#include "contenttype.hpp"
#include "account.pb.h"
#include "common.pb.h"
using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(accountPort, 8007, "服务端口");
DEFINE_string(stringPort, "8007", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小


namespace accountService
{

	BitMap bitmap;

	class accountServiceImpl : public accountService
	{ // 登录服务
	public:
		accountServiceImpl(){};
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
			if (-1 == get_user_by_id(bufUser, request->userid()))
			{ // 请求登录的账户不存在
				LOG(INFO) << endl
						  << control->remote_side() << " 请求登录的账号: " << request->userid() << " 不存在" << endl;
				response->set_code(-1);
				response->set_errorres("账户不存在");
				return;
			}
			if (bufUser.userPwd != request->userpwd() || bufUser.userId != request->userid())
			{ // 账号或密码有误，从数据库匹配
				LOG(INFO) << endl
						  << control->remote_side() << " 请求登录账号: " << request->userid() << " 失败,密码错误" << endl;
				response->set_code(-1);
				response->set_errorres("密码错误");
			}
			else
			{
				if (bitmap.HasExisted(atoi(request->userid().c_str())))
				{ // 请求保持账号登陆状态
					LOG(INFO) << endl
							  << control->remote_side() << " 请求保持账号: " << request->userid() << " 登陆状态，成功" << endl;
					response->set_code(1);
				}
				else
				{ // 账号、密码准确无误
					bitmap.Set(atoi(request->userid().c_str()));
					LOG(INFO) << endl
							  << control->remote_side() << " 请求登陆账号: " << request->userid() << " 成功" << endl;
					response->set_code(1);
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
			if (1 == get_user_by_id(bufUser, request->userid()) && bufUser.userId != "")
			{ // 获取数据库数据
				LOG(INFO) << endl
						  << control->remote_side() << " 请求获取账号: " << request->userid() << " 的数据,成功" << endl;
				response->set_userid(bufUser.userId);
				response->set_nickname(bufUser.userNickName);
				response->set_headimgurl(bufUser.userHeadImgUrl);
				response->set_male(bufUser.userMale);
				response->set_age(bufUser.userAge);
				response->set_profile(bufUser.userProfile);
			}
			else
			{ // 获取用户数据失败
				LOG(INFO) << endl
						  << control->remote_side() << " 请求获取账号: " << request->userid() << " 的数据,失败" << endl;
				response->set_userid("404");
				response->set_nickname("用户不存在");
				response->set_headimgurl("用户不存在");
				response->set_male(1);
				response->set_age(0);
				response->set_profile("用户不存在");
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void registerFun(google::protobuf::RpcController *control_base,
								 const registerReq *request,
								 commonService::commonResp *response,
								 google::protobuf::Closure *done)
		{ // 注册用户函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << ": " << request->userpwd()
					  << " (attached : " << control->request_attachment() << ")";
			UserInfoTable newUser = {
				request->userid(),
				"书友 " + request->userid(),
				"http://" + FLAGS_iPort + "/fileService/fileDownFun/images/default.png",
				request->userpwd(),
				"你好 这个人很懒~",
				1,
				19};
			int ins_res = insert_user(newUser);
			switch (ins_res)
			{
			case 1:
				// 注册并录入数据库成功
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: " << request->userid() << " 成功" << endl;
				response->set_code(1); // 设置返回值
				break;
			case -1:
				// 连接数据库错误
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: " << request->userid() << " 失败，数据库出错" << endl;
				response->set_code(-1); // 设置返回值
				response->set_errorres("注册失败");
				break;
			case -2:
				// 账户已存在
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: " << request->userid() << " 失败，账号已存在" << endl;
				response->set_code(-1); // 设置返回值
				response->set_errorres("账户已存在");
				break;
				// case -4:
				//  	// 密码不符要求
				//  	LOG(INFO) << endl << control->remote_side() << "请求注册账号" << request->userid() << " 失败，密码格式错误" << endl;
				//  	response->set_code(-1); // 设置返回值
				//  	response->set_errorres("密码不符合要求");
				break;
			default:
				// 注册失败
				LOG(INFO) << endl
						  << control->remote_side() << " 请求注册账号: " << request->userid() << " 失败,原因未知" << endl;
				response->set_code(-1); // 设置返回值
				response->set_errorres("注册失败");
				break;
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
			
			int ret = update_password(request->userid(),request->newpassword()) ;
			if(ret > 0 )
			{
				response->set_code(1);
				LOG(INFO) << endl
						  << control->remote_side() << "  修改密码成功" << endl;
			}else{
				response->set_code(-1);
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
			get_all_user(res); // 获得所有用户信息
			int count = 0;
			for (auto i : res)
			{ // 把所有用户信息回发用户
				++count;
				auto user = response->add_lists();
				user->set_userid(i.userId);
				user->set_nickname(i.userNickName);
				user->set_headimgurl(i.userHeadImgUrl);
				user->set_profile(i.userProfile);
				user->set_male(i.userMale);
				user->set_age(i.userAge);
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
			get_LikeUser_by_id(res, request->cond_option(), request->cond_value()); // 获得匹配用户信息
			int count = 0;
			if (!res.empty())
			{ // 匹配到对应用户信息
				for (auto i : res)
				{ // 把所有用户信息回发用户
					++count;
					auto user = response->add_lists();
					user->set_userid(i.userId);
					user->set_nickname(i.userNickName);
					user->set_headimgurl(i.userHeadImgUrl);
					user->set_profile(i.userProfile);
					user->set_male(i.userMale);
					user->set_age(i.userAge);
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
			if (!request->userid().empty())
			{ //文件类型错误 -3: 连接数据库出错 -2： 账户不存在 - 1：失败 1：成功
				UserInfoTable user ;
				int ret = get_user_by_id(user, request->userid());
				if( 1 == ret )
				{
					if( request->has_nickname())
					{
						user.userNickName = request->nickname();
					}
					if( request->has_profile())
					{
						user.userProfile = request->profile();
					}
					if( request->has_male())
					{
							user.userMale = request->male();
					}
					if( request->has_age())
					{
						user.userAge = request->age();
					}
					if( request->has_headimgdata())
					{
						//前端默认格式
						string suffix = "png" ;
						// if(request->has_headimgtype() )
						// {
						// 	suffix =request->headimgtype(); // 获取文件后缀
						// }else{
						// 	response->set_code(-3);
						// 	response->set_errorres("文件格式错误");
						// 	LOG(INFO) << endl
						// 		<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息,头像格式错误" << endl;
						// 	return;
						// }
						string headPath = FLAGS_imagesPath+ request->userid() + "_head." + suffix;
						cout<<"headPath is"<<headPath<<endl;
						std::ofstream outFile(headPath, std::ios::binary | std::ios::trunc);
        				outFile.write(&request->headimgdata()[0], request->headimgdata().size()); 
						string url = "http://39.105.217.90:8000/fileService/fileDownFun/images/" + request->userid() + "_head." + suffix;
						user.userHeadImgUrl =  url;
					}
					if( 1 == up_user(user) )
					{
						response->set_code(1);
						response->set_errorres("修改成功");
						LOG(INFO) << endl
								<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息,修改成功" << endl;
					}else{
						response->set_code(-2);
						response->set_errorres("数据库错误");
						LOG(INFO) << endl
								<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息失败，数据库错误" << endl;	
					}

				}else{
					response->set_code(-1);
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
			if (!request->userid().empty())
			{
				int int_res = del_user(request->userid());
				if (int_res == 1)
				{ // -4：密码不符要求 - 3：账号不存在 - 2：账户已存在 - 1：连接数据库出错 0：失败 1：成功
					response->set_code(1);
					LOG(INFO) << endl
							  << control->remote_side() << "请求注销用户账号: " << request->userid() << " 成功" << endl;
				}
				else if (int_res == -3)
				{
					response->set_code(-1);
					response->set_errorres("该账号不存在或已被注销");
					LOG(INFO) << endl
							  << control->remote_side() << "请求注销用户账号: " << request->userid() << " 失败,账号不存在" << endl;
				}
				else
				{
					response->set_code(-1);
					response->set_errorres("出错了，请重试");
					LOG(INFO) << endl
							  << control->remote_side() << "请求注销用户账号: " << request->userid() << " 失败" << endl;
				}
			}
			else
			{
				response->set_code(-1);
				response->set_errorres("请输入要注销的id");
				LOG(INFO) << endl
						  << control->remote_side() << "请求注销用户账号失败,未输入账号" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}

