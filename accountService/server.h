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
#include "bookcity.pb.h"
#include "bookshelf.pb.h"
#include "collectdata.pb.h"
#include "common.pb.h"
#include "echo.pb.h"
#include "fileupdown.pb.h"
using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(port, 8000, "服务端口");
DEFINE_string(stringPort, "8000", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

namespace echoService
{

	class EchoServiceImpl : public EchoService
	{ // 测试服务
	public:
		EchoServiceImpl(){};
		virtual ~EchoServiceImpl(){};
		virtual void EchoFun(google::protobuf::RpcController *control_base,
							 const EchoRequest *request,
							 EchoResponse *response,
							 google::protobuf::Closure *done)
		{ // 测试函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			LOG(INFO) << endl
					  << control->remote_side() << " 测试消息: " << request->message();
			response->set_message(request->message()); // 设置回发消息
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}

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
						string type = control->http_request().content_type() ;
						int index = type.find_last_of('.') ;
						cout<<index<<endl ;
						string suffix;
						if( 0 <= index && index < type.length() )
						{
							suffix = suffix.substr(index); // 获取文件后缀
						}else{
							response->set_code(-3);
							response->set_errorres("文件格式错误");
							LOG(INFO) << endl
								<< control->remote_side() << "请求修改账号: " << request->userid() << " 个人信息,头像格式错误" << endl;
							return;
						}
						string headPath = FLAGS_imagesPath+ request->userid() + "_head." + suffix;
						std::ofstream outFile(headPath, std::ios::binary | std::ios::trunc);
        				outFile.write(&request->headimgdata()[0], request->headimgdata().size()); 
						string url = "http://39.105.217.90:8000/fileService/fileDownFun/images/" + request->userid() + "_head" + suffix;
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

namespace commonService
{

	class commonServiceImpl : public commonService
	{ // 评论、点赞
	public:
		commonServiceImpl(){};
		virtual ~commonServiceImpl(){};
		virtual void addCommentFun(google::protobuf::RpcController *control_base,
								   const commentItem *request,
								   commonResp *response,
								   google::protobuf::Closure *done)
		{ // 添加评论信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			string bookId = request->bookid();
			int commentId = get_max_commentid();		  // 每次获得最大可用的值
			string hitId = "hit_" + to_string(commentId); // 点赞id
			int pageNum = 0;							  // 默认0，表示对整本书评价
			if (request->has_pagenum())
			{ // 对页评价
				pageNum = request->pagenum();
			}
			int parentId = 0; // 默认0, 顶层评论
			if (request->has_parentid())
			{ // 评论回复
				parentId = request->parentid();
			}
			string time = request->timestamp();
			string content = request->commentcontent();
			string userId = request->userid();
			UserInfoTable bufUser;
			string userName = "匿名用户";
			string userHead = "http://" + FLAGS_iPort + "/fileService/fileDownFun/images/default.png";
			if (1 == get_user_by_id(bufUser, userId) && bufUser.userId != "")
			{ // 获取数据库数据
				userName = bufUser.userNickName;
				userHead = bufUser.userHeadImgUrl;
			}
			CommentTable comment{commentId, bookId, pageNum, parentId, time, content, hitId, userId, userName, userHead};
			int retSQL = insertCommentSQL(comment);
			int retJSON = insertCommentData(comment);
			if (retSQL < 1 && retJSON < 1)
			{
				response->set_code(-1);
				response->set_errorres("评论失败，请重试");
			}
			else
			{
				if (retSQL < 1)
				{
					LOG(INFO) << endl
							  << control->remote_side() << " 数据库添加评论信息失败";
				}
				else if (retJSON < 1)
				{
					LOG(INFO) << endl
							  << control->remote_side() << " JSON添加评论信息失败";
				}
				response->set_code(1);
				response->set_errorres("评论成功");
				LOG(INFO) << endl
						  << control->remote_side() << "评论成功";
				cout << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void delCommentFun(google::protobuf::RpcController *control_base,
								   const DelCommentReq *request,
								   commonResp *response,
								   google::protobuf::Closure *done)
		{ // 删除评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			// 验证用户操作
			string userId = request->userid();
			int ret = del_comment(request->commentid());
			if (ret < 1)
			{
				response->set_code(0);
				response->set_errorres("删除评论信息失败");
				LOG(INFO) << endl
						  << control->remote_side() << "删除评论信息失败" << endl;
				return;
			}
			response->set_code(1);
			LOG(INFO) << endl
					  << control->remote_side() << "删除评论信息成功" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getCommentFun(google::protobuf::RpcController *control_base,
								   const getCommentReq *request,
								   commentListResp *response,
								   google::protobuf::Closure *done)
		{ // 查询用户评价的内容
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int ret = 0;
			int count = 0;
			if (!request->has_userid())
			{
				// 验证用户
			}
			unordered_map<int, commentItem *> allCommentItems;
			if (request->has_bookid())
			{
				if (request->has_pagenum())
				{ // 查询对某页所有评价
					vector<CommentTable> res;
					ret = get_page_comment(request->bookid(), request->pagenum(), res);
					for (auto &i : res)
					{
						int parentId = i.parentId;
						int commentId = i.commentId;
						commentItem *comment = NULL;
						if (0 == parentId)
						{ // 顶层评论
							comment = response->add_lists();
							++count;
						}
						else
						{ // 回复评论
							comment = allCommentItems[parentId]->add_childcomments();
							allCommentItems[parentId]->set_childcount(allCommentItems[parentId]->childcount() + 1);
						}
						comment->set_commentid(i.commentId);
						comment->set_parentid(i.parentId);
						comment->set_timestamp(i.timeStamp);
						comment->set_commentcontent(i.content);
						comment->set_userid(i.userId);
						comment->set_nickname(i.userName);
						comment->set_headurl(i.userHead);
						comment->set_childcount(0);
						allCommentItems.insert(pair<int, commentItem *>(commentId, comment));
					}
					if (0 < ret)
						LOG(INFO) << endl
								  << control->remote_side() << "查询对某页评价成功" << endl;
					else
						LOG(INFO) << endl
								  << control->remote_side() << "查询对某页评价失败" << endl;
				}
				else
				{ // 查询对书籍评价
					vector<CommentTable> res;
					ret = get_book_comment(request->bookid(), res);
					for (auto &i : res)
					{
						int parentId = i.parentId;
						int commentId = i.commentId;
						commentItem *comment = NULL;
						if (parentId == 0)
						{
							comment = response->add_lists();
							++count;
						}
						else
						{
							comment = allCommentItems[parentId]->add_childcomments();
							allCommentItems[parentId]->set_childcount(allCommentItems[parentId]->childcount() + 1);
						}
						comment->set_commentid(i.commentId);
						comment->set_parentid(i.parentId);
						comment->set_timestamp(i.timeStamp);
						comment->set_commentcontent(i.content);
						comment->set_userid(i.userId);
						comment->set_nickname(i.userName);
						comment->set_headurl(i.userHead);
						comment->set_childcount(0);
						allCommentItems.insert(pair<int, commentItem *>(commentId, comment));
						++count;
					}
					if (0 < ret)
						LOG(INFO) << endl
								  << control->remote_side() << "查询对书籍评价成功" << endl;
					else
						LOG(INFO) << endl
								  << control->remote_side() << "查询对书籍评价失败" << endl;
				}
			}
			response->set_count(count);
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}

namespace bookCityService
{

	class bookCityServiceImpl : public bookCityService
	{
	public:
		bookCityServiceImpl(){};
		virtual ~bookCityServiceImpl(){};
		virtual void addBookInfoFun(google::protobuf::RpcController *control_base,
									const bookInfoItem *request,
									commonService::commonResp *response,
									google::protobuf::Closure *done)
		{ // 添加书籍信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int ret = insert_book(request->bookid(), request->bookname(), request->bookheadurl(), request->bookdownurl(), request->authorname(), request->booktype());
			if (ret < 1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(0);
				response->set_errorres("add faild");
				LOG(INFO) << control->remote_side() << "导入书籍失败" << endl;
			}
			else
			{
				response->set_code(1);
				response->set_errorres("sucessful");
				LOG(INFO) << endl
						  << control->remote_side() << "导入书籍成功" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void searchBookInfoFun(google::protobuf::RpcController *control_base,
									   const searchBookReq *request,
									   booksRespList *response,
									   google::protobuf::Closure *done)
		{ // 搜索某本书相关信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";

			int ret = -1; //保存查找数目
			if (request->has_bookid())
			{
				BookInfoTable bookres;
				ret = get_book_by_id(bookres, request->bookid());
				if (ret != -1)
				{
					response->set_count(ret);
					auto book = response->add_lists();
					book->set_bookid(bookres.bookId);
					book->set_bookname(bookres.bookName);
					book->set_bookheadurl(bookres.bookHeadUrl);
					book->set_bookdownurl(bookres.bookDownUrl);
					book->set_booktype(bookres.bookType);
					book->set_authorname(bookres.authorName);
				}
			}
			if (request->has_bookname())
			{
				vector<BookInfoTable> bookres;
				ret = get_book_by_bookname(bookres, request->bookname());
				for (int i = 0; i < ret; ++i)
				{
					response->set_count(ret);
					auto book = response->add_lists();
					book->set_bookid(bookres[i].bookId);
					book->set_bookname(bookres[i].bookName);
					book->set_bookheadurl(bookres[i].bookHeadUrl);
					book->set_bookdownurl(bookres[i].bookDownUrl);
					book->set_booktype(bookres[i].bookType);
					book->set_authorname(bookres[i].authorName);
				}
			}
			if (request->has_authorname())
			{
				vector<BookInfoTable> bookres;
				ret = get_book_by_authorname(bookres, request->authorname());
				for (int i = 0; i < ret; ++i)
				{
					response->set_count(ret);
					auto book = response->add_lists();
					book->set_bookid(bookres[i].bookId);
					book->set_bookname(bookres[i].bookName);
					book->set_bookheadurl(bookres[i].bookHeadUrl);
					book->set_bookdownurl(bookres[i].bookDownUrl);
					book->set_booktype(bookres[i].bookType);
					book->set_authorname(bookres[i].authorName);
				}
			}

			if (ret == -1)
			{
				response->set_count(-1);
				LOG(INFO) << endl
						  << control->remote_side()
						  << "未搜索到图书" << endl;
			}
			else
			{
				LOG(INFO) << endl
						  << control->remote_side()
						  << "搜索到图书" << ret << " 本。" << endl;
			}

			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void setBookInfoFun(google::protobuf::RpcController *control_base,
									const bookInfoItem *request,
									commonService::commonResp *response,
									google::protobuf::Closure *done)
		{ // 更新书籍信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int ret = up_book(request->bookid(), request->bookname(), request->bookheadurl(), request->bookdownurl(), request->booktype(), request->authorname());
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(0);
				response->set_errorres("add faild");
			}
			response->set_code(1);
			response->set_errorres("sucessful");
			LOG(INFO) << endl
					  << control->remote_side() << "更新书籍信息成功" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getAllBookInfoFun(google::protobuf::RpcController *control_base,
									   const getAllBookInfoReq *request,
									   booksRespList *response,
									   google::protobuf::Closure *done)
		{ // 获取数据库所有书籍信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			vector<BookInfoTable> res;
			int ret = get_all_book(res); // 获取所有书籍信息
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
			}
			int start = request->offset();
			int size = 0 == request->size() ? request->size() : 10;
			int i = start;
			int end = std::min(start + 10, ret);
			for (; start < ret; ++start)
			{
				auto book = response->add_lists();
				book->set_bookid(res[start].bookId);
				book->set_bookname(res[start].bookName);
				book->set_bookheadurl(res[start].bookHeadUrl);
				book->set_bookdownurl(res[start].bookDownUrl);
				book->set_booktype(res[start].bookType);
				book->set_authorname(res[start].authorName);
			}
			response->set_count(ret);
			LOG(INFO) << endl
					  << control->remote_side() << "查询所有书籍成功，本次查到 " << ret << " 本书" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void delBookInfoFun(google::protobuf::RpcController *control_base,
									const delBookReq *request,
									commonService::commonResp *response,
									google::protobuf::Closure *done)
		{ // 删除某本书
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";

			int ret = del_book(request->bookid());
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(0);
				response->set_errorres("del faild");
			}
			response->set_code(1);
			response->set_errorres("sucessful");
			LOG(INFO) << endl
					  << control->remote_side() << "删除书籍成功" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}

namespace bookShelfService
{

	class bookShelfServiceImpl : public bookShelfService
	{ // 个人书架书籍管理服务
	private:
		//判断对应文件是否存在
		bool isFileExisit(string bookId, int page)
		{
			string filePath;
			filePath = FLAGS_pdfPath + bookId + "/" + bookId + "_" + std::to_string(page) + ".pdf";
			if (eaccess(filePath.c_str(), F_OK) == -1)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

	public:
		bookShelfServiceImpl(){};
		virtual ~bookShelfServiceImpl(){};
		virtual void addToShelfFun(google::protobuf::RpcController *control_base,
								   const addShelfBookReq *request,
								   commonService::commonResp *response,
								   google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int ret = insert_shelf(request->userid(), request->bookid());
			if (ret < 1)
			{
				response->set_code(0);
				response->set_errorres("insert faild");
			}
			response->set_code(1);
			response->set_errorres("insert sucessful");
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
			return;
		}

		virtual void delFromShelfFun(google::protobuf::RpcController *control_base,
									 const delShelfBookReq *request,
									 ::bookShelfService::delShelfBookRep* response,
									 google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			int size  = request->bookids_size();
			int sus = 0, fail = 0;
			for(int i = 0 ; i < size; i++){
				if(1 == del_shelf_book(request->userid(), request->bookids(i)))
				 	sus++ ;
				else{
					response->add_failbookids(request->bookids(i).c_str());
				}
			}
			response->set_suscount(sus);
			response->set_userid(request->userid());
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 用户 : " << request->userid()
					  << " (请求删书籍"<<to_string(size)
					  <<"本，成功 : " +to_string(sus) + "本,失败" 
					  <<to_string(size -sus) + "本"  << ")";

			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		void getMyshelfFun(google::protobuf::RpcController *control_base,
						   const getShelfBooksReq *request,
						   bookCityService::booksRespList *response,
						   google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			vector<BookInfoTable> bookres;

			int ret = get_book_by_userid(bookres, request->userid());
			response->set_count(ret);
			cout << "myshelf函数：" << ret << endl;
			if (-1 != ret)
			{
				for (int i = 0; i < ret; ++i)
				{
					auto book = response->add_lists();
					book->set_bookid(bookres[i].bookId);
					book->set_bookname(bookres[i].bookName);
					book->set_bookheadurl(bookres[i].bookHeadUrl);
					book->set_bookdownurl(bookres[i].bookDownUrl);
					book->set_booktype(bookres[i].bookType);
					book->set_authorname(bookres[i].authorName);
				}
				LOG(INFO) << endl
						  << control->remote_side() << "请求获取用户 " << request->userid() << " 书架的所有书籍成功" << endl;
			}
			else
			{
				LOG(INFO) << endl
						  << control->remote_side() << "请求获取用户 " << request->userid() << " 书架的所有书籍失败，可能书架为空或用户不存在" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
		virtual void getOnePageFun(::google::protobuf::RpcController* control_base,
                       const ::bookShelfService::getPageReq* request,
                       ::bookShelfService::bookPageResp* response,
                       ::google::protobuf::Closure* done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << ", 发出请求用户 : " << request->userid()
					  << " (attached : " << control->request_attachment() << ")";
			
			string url("http://39.105.217.90:8000/fileService/fileDownFun/books/spiltedBooks/");
			if(isFileExisit(request->bookid(),request->page())){
				response->set_page(request->page() + 1);
				response->set_bookid(request->bookid());
				response->set_userid(request->userid());
				//生成url
				url = url + request->bookid() +"/"+ request->bookid()
						+"_" +to_string(request->page()) + ".pdf" ;
				response->set_url(url);
				response->set_code(1);

				LOG(INFO) << endl << control->remote_side() << "用户请求获取书籍 " 
						<< request->bookid() << "第 "<<request->page()
						<<"页，请求成功"<< endl;

			}else{
				response->set_code(-1);
				LOG(INFO) << endl << control->remote_side() << "用户请求获取书籍 " 
						<< request->bookid() << "第 "<<request->page()
						<<"页，请求失败"<< endl;
			}

		}
	};

}

namespace fileService
{

	class fileServiceImpl : public fileService
	{ // 文件传输服务
	public:
		fileServiceImpl(){};
		virtual ~fileServiceImpl(){};

		void sendFile(brpc::Controller *control, string filePath, string contentType)
		{ // 发送文件函数
			std::fstream tmpfile;
			tmpfile.open(filePath, ios::in | ios::binary | ios::ate); // 二进制输入(读取),定位到文件末尾
			if (tmpfile.is_open())
			{
				// 打开文件成功，contentType不为空
				control->http_response().set_content_type(static_cast<char *>(contentType.data()));
				size_t length = tmpfile.tellg(); // 获取文件大小
				int all_len = length;
				tmpfile.seekg(0, std::ios::beg); // 定位到流开始
				butil::IOBuf os;
				char data[IOBuf_MAX_SIZE];
				while (length >= IOBuf_MAX_SIZE)
				{
					os.clear(); // 清空流
					tmpfile.read(data, IOBuf_MAX_SIZE);
					os.append(data, IOBuf_MAX_SIZE);
					control->response_attachment().append(os);
					length -= IOBuf_MAX_SIZE;
				}
				if (length > 0)
				{
					os.clear(); // 清空流
					tmpfile.read(data, length);
					os.append(data, length);
					control->response_attachment().append(os);
					length = 0;
				}
				LOG(INFO) << "请求下载文件: " << filePath << " 成功" << endl;
				control->http_response().set_status_code(200);
			}
			else
			{ // 读取失败
				LOG(INFO) << " 失败," << filePath << "打开文件失败" << endl;
				control->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
				control->http_response().set_status_code(404);
			}
		}

		void fileDownFun(google::protobuf::RpcController *control_base,
						 const HttpRequest *,
						 HttpResponse *,
						 google::protobuf::Closure *done)
		{ // 下载文件函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			if (!control->http_request().unresolved_path().empty())
			{
				string filePath = FLAGS_dataPath + control->http_request().unresolved_path();
				cout<<"filepath  is "<<filePath<<endl ;
				string suffix = filePath.substr(filePath.find_last_of('.')); // 获取文件后缀
				sendFile(control, filePath, getContentType(suffix));
			}
			else
			{
				control->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
				control->http_response().set_status_code(404);
			}
		}
	};

}

namespace collectdataService
{

	class collectDataServiceImpl : public collectService
	{
	private :

		unsigned long findLocate(const string & str ,const char & ch)
		{
			unsigned size = str.size() ;
			for(unsigned long i = 0 ; i < size ;i ++ ){
				if( str[i] == ch)
					return i ;
			}
		return -1 ;
		}

	public:
		collectDataServiceImpl(){};
		virtual ~collectDataServiceImpl(){};

		//收集阅读数据 包括单页时间和视线点时间
		virtual void collectReadDataFun(google::protobuf::RpcController *control_base,
										const readBookRecored *request,
										commonService::commonResp *response,
										google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			string bookId = request->bookid();
			string userId = request->userid();
			string startTime = request->starttime();
			string endTime = request->endtime();
			int pageNum = request->pagenum();
			int size = request->lists_size();
			vector<oneSight> lists;
			oneSight buffer;
			for (int index = 0; size > 0 && index < size; ++index)
			{
				buffer.x = request->lists(index).x();
				buffer.y = request->lists(index).y();
				buffer.timeStamp = request->lists(index).timestamp();
				lists.push_back(buffer);
			}
			pageSight bufSight{userId, bookId, pageNum, lists, startTime, endTime};
			int ret = insertSightData(bufSight);
			if (ret)
			{
				response->set_code(1);
				response->set_errorres("succeed");
				LOG(INFO) << endl
						  << control->remote_side() << "新增用户阅读视线数据，成功" << endl;
			}
			else
			{
				response->set_code(-1);
				response->set_errorres("error");
				LOG(INFO) << endl
						  << control->remote_side() << "新增用户阅读视线数据，失败" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		// 获取区间阅读统计数据
		virtual void getReadTimeCount(google::protobuf::RpcController *control_base,
									  const readCountReq *request,
									  readCountResp *response,
									  google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";

			//一天的专注度统计
			//数据库读取写入csv
			int csvRet = storageAnalyseCsv(request->daytime(),request->userid());
			//当天没有数据
			if( csvRet == -1){
				response->mutable_status()->set_code(-1);
				response->mutable_status()->set_errorres("今日未读书");
				LOG(INFO)  << "(客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  <<"请求" << request->daytime() << "的阅读分析数据失败)";
				return;
			}
			//当天有数据
			storageAnalyseJson(request->userid());
			//结果从json获取
			map<string,string>time_focus ;
			//散点 <action+color,points>
			vector<map<string,vector<map<double,double>>>>scatterDiagram;
			//line Chart
			vector<double>lineChart ;
			int ret = getAnalyseResult(time_focus,scatterDiagram,lineChart,request->daytime(),request->userid());

			//获取散点  折线图
			cout<<"ret = "<<ret<<endl;
			if( ret == 143 ){
				response->mutable_thistimedata()->set_hour(stoi(time_focus["hour"].c_str()));
				response->mutable_thistimedata()->set_min(stoi(time_focus["min"].c_str()));
				response->mutable_thistimedata()->set_sec(stoi(time_focus["sec"].c_str()));
				response->mutable_thistimedata()->set_focus( atof(time_focus["focus"].c_str() ) ) ;
				response->mutable_thistimedata()->set_pages(stoi(time_focus["pages"].c_str()));
				response->mutable_thistimedata()->set_rows(stoi(time_focus["rows"].c_str()));
				//散点图
				string x ,y ;
				int lx,ly ;
				for(int i = 0 ; i < 3 ; i++	){
					auto pointype =  response->add_scatterdiagram();
					unsigned loc = findLocate(scatterDiagram[i].begin()->first,'+');
					pointype->set_action((scatterDiagram[i].begin()->first).substr(0,loc));//提取action
					pointype->set_color(
						scatterDiagram[i].begin()->first.
							substr(loc + 1,scatterDiagram[i].begin()->first.size() - loc));//提取color
					for(int j = 0 ; j < 20 ;j++ ){
						auto point = pointype->add_locate();
						x = to_string( (scatterDiagram[i].begin()->second)[j].begin()->first ) ;
						y = to_string( (scatterDiagram[i].begin()->second)[j].begin()->second) ; 
						lx = x.rfind(".");
						ly = y.rfind(".");
						point->set_x(  atof((x.substr(0,lx)).c_str()) );
						point->set_y(  atof((y.substr(0,ly)).c_str()) );
					}
				}
				//折线图
				for(int i = 0 ; i < 11 ; i++){
					auto speedPoints = response->add_speedpoints();
					speedPoints->set_point(lineChart[i])  ;
				}
			}

			//返回
			if(ret == 143)
			{
				//12时段
				string userId = request->userid();
				string dayTime = request->daytime();
				float intervals[12] = {0};
				for (int i = 0; i < size(intervals); ++i)
				{
					intervals[i] = 0;
				}
				getIntervalCount(userId, dayTime, intervals);
				for (int i = 0; i < size(intervals); ++i)
				{
					auto minute = response->add_lists();
					minute->set_min((int)intervals[i]);
				}
				response->mutable_status()->set_code(1);
				LOG(INFO)  << "(客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  <<"请求" << request->daytime() << "的阅读分析数据成功)";
			}
			else
			{
				response->mutable_status()->set_code(-1);
				response->mutable_status()->set_errorres("今日未读书");
				LOG(INFO)  << "(客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  <<"请求" << request->daytime() << "的阅读分析数据失败)";
			}

			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}
