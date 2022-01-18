#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "public/location.hpp"
#include "public/service.hpp"
#include "database/pageCommentSql.hpp"
#include "common.pb.h"
#include "pagecomment.pb.h"

using namespace std;
using namespace ormpp;
using namespace service;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(commonPort, 8008, "服务端口");
DEFINE_string(stringPort, "8008", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小


namespace PageCommentService
{
	class PageCommentServiceImpl : public PageCommentService
	{ 
	private:
		PageCommentImpl __pageCommentSql;
		int __current_max_comment_id;
	public:
		PageCommentServiceImpl()
		{
			__current_max_comment_id = -1 ;
			__pageCommentSql.get_max_commentId(__current_max_comment_id);
			cout<<endl<<" max comment id  is "<<__current_max_comment_id<<endl ;
			if( -1 == __current_max_comment_id)
			{
				LOG(WARNING) << "获取当前最大评论id失败"<<endl ;
				throw "start page comment service failed" ;
			}
		}
		// void getTopCommentFun(google::protobuf::RpcController *control_base,
		// 				const ::PageCommentService::pageCommentReq* request,
		// 				::PageCommentService::pageCommentResp* response,
		// 				::google::protobuf::Closure* done)
		// {//获取页的父评论
		// 	brpc::ClosureGuard done_guard(done);
		// 	brpc::Controller *control =
		// 		static_cast<brpc::Controller *>(control_base);

		// 	LOG(INFO) <<endl
		// 			  << "\n收到请求[log_id=" << control->log_id()
		// 			  << "] 客户端ip+port: " << control->remote_side()
		// 			  << " 应答服务器ip+port: " << control->local_side()
		// 			  << " (attached : " << control->request_attachment() << ")"
		// 			  <<endl;
		// }
		
		// void getSubCommentFun(google::protobuf::RpcController *control_base,
		// 				const ::PageCommentService::pageCommentReq* request,
		// 				::PageCommentService::pageCommentResp* response,
		// 				::google::protobuf::Closure* done)
		// {//获取页的子评论
		// 	brpc::ClosureGuard done_guard(done);
		// 	brpc::Controller *control =
		// 		static_cast<brpc::Controller *>(control_base);

		// 	LOG(INFO) <<endl
		// 			  << "\n收到请求[log_id=" << control->log_id()
		// 			  << "] 客户端ip+port: " << control->remote_side()
		// 			  << " 应答服务器ip+port: " << control->local_side()
		// 			  << " (attached : " << control->request_attachment() << ")"
		// 			  <<endl;
		// }

		void hitCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::hitCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//点赞评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
		}

		void CancalHitCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::hitCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//取消点赞
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
		}

		void addTopCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::addCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//添加顶部评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
			//信息判断
			if(request->bookid() == "" || request->userid() < MIN_ACCOUNT ||
				request->title() == "" || request->content() == "" ||
				request->remarktime() == "" || request->page() < 0)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" userId "<<request->userid() 
					  <<" title "<<request->title()
					  <<" content "<<request->content()
					  <<" remarkTime "<<request->remarktime()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}

			PageCommentInfoTable comment ;
			
			comment.commentId = __current_max_comment_id ; // 将自增
			comment.parentId = 0;
			comment.reviewer = request->userid();
			comment.title = request->title();
			comment.content = request->content();
			comment.remarkTime = request->remarktime();
			comment.hitCount = 0 ;//默认0
			comment.bookId = request->bookid();
			comment.page = request->page();
			comment.replyCount = 0 ; //默认0

			SQL_STATUS ret = __pageCommentSql.add_comment(comment);
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 插入页顶层评论成功 :"
					  <<" reviewer "<<request->userid() 
					  <<" page "<<request->page()
					  <<" bookId "<<request->bookid()
					  <<" commentId "<<__current_max_comment_id
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				__current_max_comment_id++ ; //成功 自增
			}
			else
			{
				LOG(INFO) <<endl
					  <<" 插入页顶层评论失败 :"
					  <<" reviewer "<<request->userid() 
					  <<" page "<<request->page()
					  <<" bookId "<<request->bookid()
					  <<" remarkTime "<<request->remarktime()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("add supper comment failed !");
			}


		}

		void addSubCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::addCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//添加子评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
			
			//信息判断
			if(request->bookid() == "" || request->userid() < MIN_ACCOUNT ||
				request->title() == "" || request->content() == "" ||
				request->remarktime() == "" || !request->has_parentid()  || 
				request->page() < 0 ||
				__pageCommentSql.is_exist_supper_comment(request->parentid())  != 1 )//是否存在父评论
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" userId "<<request->userid() 
					  <<" title "<<request->title()
					  <<" content "<<request->content()
					  <<" parentid "<<request->parentid()
					  <<" remarkTime "<<request->remarktime()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}

			PageCommentInfoTable comment ;
			
			comment.commentId = __current_max_comment_id ; // 将自增
			comment.parentId = request->parentid() ;
			comment.reviewer = request->userid();
			comment.title = request->title();
			comment.content = request->content();
			comment.remarkTime = request->remarktime();
			comment.hitCount = 0 ;//默认0
			comment.bookId = request->bookid();
			comment.page = request->page();
			comment.replyCount = 0 ; //默认0

			SQL_STATUS ret = __pageCommentSql.add_comment(comment);
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 插入子评论成功 :"
					  <<" reviewer "<<request->userid() 
					  <<" parentId "<<request->parentid()
					  <<" commentId "<<__current_max_comment_id
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				__current_max_comment_id++ ; //成功 自增
			}
			else
			{
				LOG(INFO) <<endl
					  <<" 插入子评论失败成功 :"
					  <<" reviewer "<<request->userid() 
					  <<" parentId "<<request->parentid()
					  <<" remarkTime "<<request->remarktime()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("add sub comment failed !");
			}


		}

		void delCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::deleteCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//删除评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
			//信息判断
			if( request->type() < 0 || request->type() > 1 ||  //类型判断
				(request->type() == 1 && __pageCommentSql.is_existing(request->commentid() ) == false ) || 			// 子评论存在判断
				(request->type() == 0 && __pageCommentSql.is_exist_supper_comment(request->commentid() ) == false ) 	// 父评论存在判断
			)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 commentId :"<<request->commentid()
					  <<" type "<<request->type()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}
			
			SQL_STATUS ret = __pageCommentSql.delete_comment(request->commentid(), request->type());
			if(ret != SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 删除评论失败 :"
					  <<" commentId "<<request->commentid() 
					  <<" type "<<request->type()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("delete comment failed !");
			}	
			else
			{
				LOG(INFO) <<endl
					  <<" 删除评论成功 :"
					  <<" commentId "<<request->commentid() 
					  <<" type "<<request->type()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
			}		
		}
		
	};

}
