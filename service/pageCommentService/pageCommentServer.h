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
		bool isPattern(const int pattern)
		{
			if(0<= pattern && pattern<= 1)
				return true;
			else
				return false;
		}

		inline void fullComment(::PageCommentService::pageCommentInfo* comment,
								const pageCommentRes & commentSql,const int & observer)
		{
			comment->set_title(commentSql.title);
			comment->set_content(commentSql.content);
			comment->set_reviewer(commentSql.reviewer);
			comment->set_hitcount(commentSql.hitCount);
			comment->set_remarktime(commentSql.remarkTime);
			comment->set_replycount(commentSql.replyCount);
			comment->set_nickname(commentSql.nickname);
			comment->set_commentid(commentSql.commentId);
			comment->set_ishited(commentSql.isHited);
			string userHeadUrl;
			if(commentSql.isUpdateHead == 1)
				userHeadUrl = FLAGS_headUrlPre +  to_string(observer) + "_head.png";
			else
				userHeadUrl = FLAGS_headUrlPre + "default.png";
			comment->set_headurl(userHeadUrl);
		}
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
		void getTopCommentFun(google::protobuf::RpcController *control_base,
						const ::PageCommentService::pageCommentReq* request,
						::PageCommentService::pageCommentResp* response,
						::google::protobuf::Closure* done)
		{//获取页的父评论
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
			if( request->observer() < MIN_ACCOUNT || request->offset()< 0 ||
				request->count() < 0 || request->bookid().size() < 20 ||//暂定
				isPattern(request->pattern()) == false
			)
			{
				LOG(INFO) <<endl
					  <<" 请求书页父评论 字段错误 bookId :"<<request->bookid()
					  <<" observer "<<request->observer() 
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<endl;
				response->set_count(0);
				response->set_errorres("illegal information");
				return;
			} 

			SQL_STATUS ret ;
			vector<pageCommentRes>res ;
			pageCommentParameter para ;
			para.bookId = request->bookid();
			para.page = request->page();
			para.observer = request->observer();
			para.offset = request->offset();
			para.count = request->count();
			para.reverse = request->reverse();
			

			if(request->pattern() == 0)
			{//时间
				ret = __pageCommentSql.get_supper_comment_by_time(res,para);
			}
			else if(request->pattern() == 1)
			{//点赞
				ret = __pageCommentSql.get_supper_comment_by_hit(res,para);
			}

			//结果判断
			if(ret != SQL_STATUS::EXE_sus || res.size() == 0)
			{
				response->set_count(0);
				response->set_errorres("query information failed");
				LOG(INFO) <<endl
					  <<"获取书页父评论失败  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" reverse "<<request->reverse()
					  <<" page "<<request->page()
					  <<endl;
				return ;
			}
			for(auto  temp : res)
			{
				if(ret != SQL_STATUS::EXE_sus)
					continue;
				auto comment = response->add_lists();
				fullComment(comment,temp,request->observer());
			}
			response->set_count( res.size());
			LOG(INFO) <<endl
					  <<"获取书页父评论成功  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" page "<<request->page()
					  <<" reverse "<<request->reverse()<<endl;	

		}
		
		void getSubCommentFun(google::protobuf::RpcController *control_base,
						const ::PageCommentService::pageCommentReq* request,
						::PageCommentService::pageCommentResp* response,
						::google::protobuf::Closure* done)
		{//获取页的子评论
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
			if( request->observer() < MIN_ACCOUNT || request->offset()< 0 ||
				request->count() < 0 || request->bookid().size() < 20 ||//暂定
				isPattern(request->pattern()) == false || 
				__pageCommentSql.is_exist_supper_comment(request->parentid() ) != 1
			)
			{
				LOG(INFO) <<endl
					  <<" 请求书页子评论 bookId :"<<request->bookid()
					  <<" observer "<<request->observer() 
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" parentId "<<request->parentid()
					  <<endl;
				response->set_count(0);
				response->set_errorres("illegal information");
				return;
			} 

			SQL_STATUS ret ;
			vector<pageCommentRes>res ;
			pageCommentParameter para ;
			para.bookId = request->bookid();
			para.page = request->page();
			para.observer = request->observer();
			para.offset = request->offset();
			para.count = request->count();
			para.reverse = request->reverse();
			para.parentId = request->parentid();
			

			if(request->pattern() == 0)
			{//时间
				ret = __pageCommentSql.get_sub_comment_by_time(res,para);
			}
			else if(request->pattern() == 1)
			{//点赞
				ret = __pageCommentSql.get_sub_comment_by_hit(res,para);
			}
			//结果判断
			if(ret != SQL_STATUS::EXE_sus || res.size() == 0)
			{
				response->set_count(0);
				response->set_errorres("query information failed");
				LOG(INFO) <<endl
					  <<"获取书页子评论失败  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" reverse "<<request->reverse()
					  <<" page "<<request->page()
					  <<" parentId "<<request->parentid()
					  <<endl;
				return ;
			}
			for(auto  temp : res)
			{
				if(ret != SQL_STATUS::EXE_sus)
					continue;
				auto comment = response->add_lists();
				fullComment(comment,temp,request->observer());
			}
			response->set_count( res.size());
			LOG(INFO) <<endl
					  <<"获取书页子评论成功  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" page "<<request->page()
					  <<" parentId "<<request->parentid()
					  <<" reverse "<<request->reverse()<<endl;
		}

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
			//信息判断
			if( request->hitter() < MIN_ACCOUNT || request->praised()< MIN_ACCOUNT ||
				request->page() < 0 || request->bookid().size() < 20 ||//暂定
				__pageCommentSql.is_existing(request->commentid() ) != 1 ||
				__pageCommentSql.is_hit_commented(request->hitter(),request->commentid()) == 1 //已经点赞
			)
			{
				LOG(INFO) <<endl
					  <<" 请求点赞书页评论字段错误 userId :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			} 

			PageCommentHitInfoTable para ;
			para.commentId = request->commentid();
			para.hitter = request->hitter();
			para.bookId = request->bookid();
			para.page = request->page();
			para.praised = request->praised();
			SQL_STATUS ret = __pageCommentSql.hit_comment(para);

			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 书页评论点赞成功 hitter :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<" bookId : "<<request->bookid()
					  <<" page: "<<request->page()
					  <<" praised: "<<request->praised()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
			}
			else
			{
				LOG(INFO) <<endl
					  <<" 书页评论点赞失败 hitter :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<" bookId : "<<request->bookid()
					  <<" page: "<<request->page()
					  <<" praised: "<<request->praised()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
			}
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
			//信息判断
			if( request->hitter() < MIN_ACCOUNT || request->praised()< MIN_ACCOUNT ||
				request->page() < 0 || request->bookid().size() < 20 || //暂定
				__pageCommentSql.is_existing(request->commentid() ) != 1 ||
				__pageCommentSql.is_hit_commented(request->hitter(),request->commentid()) == 0 //未点赞
			)
			{
				LOG(INFO) <<endl
					  <<" 请求取消点赞书页评论字段错误 userId :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			} 

			SQL_STATUS ret = __pageCommentSql.cancal_hit_comment_by_commentId_hitter(request->commentid(),request->hitter());

			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 取消书页点赞成功 hitter :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
			}
			else
			{
				LOG(INFO) <<endl
					  <<" 取消书页点赞失败 hitter :"<<request->hitter() 
					  <<" commentId :"<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
			}
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
					  <<" 请求添加书页子评论字段错误 bookId :"<<request->bookid()
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
					  <<" 插入书页子评论成功 :"
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
					  <<" 插入书页子评论失败成功 :"
					  <<" reviewer "<<request->userid() 
					  <<" parentId "<<request->parentid()
					  <<" remarkTime "<<request->remarktime()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("add sub comment failed !");
			}


		}

		void delSubCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::deleteCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//删除子评论
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
			if( request->has_parentid() == false 	//字段被填充
				|| request->parentid() <= 0	
				|| __pageCommentSql.is_existing(request->commentid() ) != 1   // 子评论存在判断
				||  __pageCommentSql.is_exist_supper_comment(request->parentid() ) != 1 	// 父评论存在判断
			)
			{
				LOG(INFO) <<endl
					  <<" 删除书页子评论字段错误 commentId :"<<request->commentid()
					  <<" parentId "<<request->parentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}
			
			SQL_STATUS ret = __pageCommentSql.delete_sub_comment(request->commentid(), request->parentid());
			if(ret != SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 删除书页子评论失败 :"
					  <<" commentId "<<request->commentid() 
					  <<" parentId "<<request->parentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("delete sub comment failed !");
			}	
			else
			{
				LOG(INFO) <<endl
					  <<" 删除书页子评论成功 :"
					  <<" commentId "<<request->commentid() 
					  <<" parentId "<<request->parentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
			}		
		}
	
		void delSupperCommentFun(google::protobuf::RpcController *control_base,
							const ::PageCommentService::deleteCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//删除父评论
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
			if(  __pageCommentSql.is_existing(request->commentid() ) != 1 	// 父评论存在判断
			)
			{
				LOG(INFO) <<endl
					  <<" 删除书页父评论字段错误 commentId :"<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}
			
			SQL_STATUS ret = __pageCommentSql.delete_supper_comment(request->commentid());
			if(ret != SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<" 删除书页父评论失败 :"
					  <<" commentId "<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("delete supper comment failed !");
			}	
			else
			{
				LOG(INFO) <<endl
					  <<" 删除书页父评论成功 :"
					  <<" commentId "<<request->commentid()
					  <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
			}		
		}
	};

}
