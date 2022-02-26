#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "public/location.hpp"
#include "public/service.hpp"
#include "database/bookCommentSql.hpp"
#include "common.pb.h"
#include "bookcomment.pb.h"

using namespace std;
using namespace ormpp;
using namespace service;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(commonPort, 8005, "服务端口");
DEFINE_string(stringPort, "8005", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小


namespace BookCommentService
{
	//返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
	class BookCommentServiceImpl : public BookCommentService
	{ 
	private:
		inline bool isSqlOption(const int & option)
		{//模式 0:按照时间 1:按照点赞数 2:按照评分
			if( 0 <= option && option <= 2)
				return true;
			else
				return false;
		}
		//返回给调用层的结果  标题、具体内容、评分、userid、昵称、是否更改头像,时间,bookId、是否已经点赞、被点赞数
    	//typedef tuple<string,string,int,int,string,int,string,string,bool,int>  commentRes;
		inline void fullComment(::BookCommentService::bookCommentInfo* comment,const commentRes & commentSql)
		{//填充评论信息
			comment->set_title(get<0>(commentSql));
			comment->set_content(get<1>(commentSql));
			comment->set_score(get<2>(commentSql));
			string  userHeadUrl ;//头像
			if(get<5>(commentSql) == 1)
				userHeadUrl = FLAGS_headUrlPre +  to_string(get<3>(commentSql)) + "_head.png";
			else
				userHeadUrl = FLAGS_headUrlPre + "default.png";
			comment->set_headurl(userHeadUrl);
			comment->set_nickname(get<4>(commentSql));
			comment->set_praised(get<3>(commentSql));
			comment->set_remarktime(get<6>(commentSql));
			comment->set_bookid(get<7>(commentSql));
			comment->set_ishit(get<8>(commentSql));
			comment->set_hitcount(get<9>(commentSql));
		}
	private:
		BookCommentImpl __bookCommentSql;

	public:
		virtual void getBookCommentFun(::google::protobuf::RpcController* control_base,
                       const ::BookCommentService::bookCommentReq* request,
                       ::BookCommentService::bookCommentResp* response,
                       ::google::protobuf::Closure* done)
		{//获取评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求评论 "<<endl;
			
			//信息判断
			if(request->bookid() == "" || request->observer() < MIN_ACCOUNT || 
					request->count() < 0 || request->offset() < 0 || 
					isSqlOption(request->pattern()) == false
				)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" observer "<<request->observer() 
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()<<endl;
				response->set_count(0);
				response->set_errorres("illegal information");
				return;
			}
			SQL_STATUS ret ;
			vector<commentRes> res;
			if(request->pattern() == 0)
			{//时间
				ret = __bookCommentSql.get_comment_by_time(request->observer() ,
							request->offset(), request->count(), res,request->bookid(),request->reverse() );
			}
			else if(request->pattern() == 1)
			{//点赞
				ret = __bookCommentSql.get_comment_by_hit(request->observer() ,
							request->offset(), request->count(), res,request->bookid(),request->reverse() );
			}
			else
			{//评分
				ret = __bookCommentSql.get_comment_by_score(request->observer() ,
							request->offset(), request->count(), res,request->bookid(),request->reverse() );
			}
			//结果判断
			if(ret != SQL_STATUS::EXE_sus || res.size() == 0)
			{
				response->set_count(0);
				response->set_errorres("query information failed");
				LOG(INFO) <<endl
					  <<"获取书籍评论失败  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" reverse "<<request->reverse()<<endl;
				return ;
			}
			for(auto  temp : res)
			{
				if(ret != SQL_STATUS::EXE_sus)
					continue;
				auto comment = response->add_lists();
				fullComment(comment,temp);
			}
			response->set_count( res.size());
			LOG(INFO) <<endl
					  <<"获取书籍评论成功  observer: " <<request->observer()
					  <<" bookId : "<<request->bookid()
					  <<" count "<<request->count()
					  <<" offset "<<request->offset()
					  <<" pattern "<<request->pattern()
					  <<" reverse "<<request->reverse()<<endl;		
		}

		virtual void hitCommentFun(::google::protobuf::RpcController* control_base,
							const ::BookCommentService::hitCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//评论点赞
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求点赞书籍评论 "<<endl;
			//信息判断
			if(request->bookid() == "" || request->hitter() < MIN_ACCOUNT || request->praised() < MIN_ACCOUNT)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" hitter "<<request->hitter() 
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}

			SQL_STATUS ret = __bookCommentSql.hit_comment_by_bookId_praised(request->hitter(),request->bookid(),request->praised());
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<"书籍评论点赞成功  hitter: " <<request->hitter()
					  <<" bookId : "<<request->bookid()
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
				response->set_errorres("add book comment hit sus");
			}
			else
			{
				LOG(INFO) <<endl
					  <<"书籍评论点赞失败  hitter: " <<request->hitter()
					  <<" bookId : "<<request->bookid()
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
				response->set_errorres("add book comment hit fail");
			}
		}
		
		virtual void CancalHitCommentFun(::google::protobuf::RpcController* control_base,
							const ::BookCommentService::hitCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//取消评论点赞
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求取消书籍评论点赞 "<<endl;

			//信息判断
			if(request->bookid() == "" || request->hitter() < MIN_ACCOUNT || request->praised() < MIN_ACCOUNT)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" hitter "<<request->hitter() 
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}

			SQL_STATUS ret = __bookCommentSql.cancal_hit_comment_by_bookId_praised(request->hitter(),request->bookid(),request->praised());
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<"取消书籍评论点赞成功  hitter: " <<request->hitter()
					  <<" bookId : "<<request->bookid()
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
				response->set_errorres("delete book comment hit sus");
			}
			else
			{
				LOG(INFO) <<endl
					  <<"取消书籍评论点赞失败  hitter: " <<request->hitter()
					  <<" bookId : "<<request->bookid()
					  <<" praised "<<request->praised()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
				response->set_errorres("delete book comment  hit fail");
			}
		}
		
		virtual void addCommentFun(::google::protobuf::RpcController* control_base,
							const ::BookCommentService::addCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//添加评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求添加书籍评论 "<<endl;
			//信息判断
			if(request->bookid() == "" || request->userid() < MIN_ACCOUNT ||
				request->title() == "" || request->content() == "" ||
				request->score() < 0 || request->remarktime() == "")
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" userId "<<request->userid() 
					  <<" title "<<request->title()
					  <<" content "<<request->content()
					  <<" score "<<request->score()
					  <<" remarkTime "<<request->remarktime()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}
			//书籍评论仅仅覆盖
			__bookCommentSql.delete_comment(request->bookid(),request->userid());
			SQL_STATUS ret = __bookCommentSql.add_comment(
											request->bookid(),request->userid(),request->score(),//整型保存
											request->title(),request->content(),request->remarktime()
										);
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<"添加书籍评论成功  userId: " <<request->userid()
					  <<" bookId : "<<request->bookid()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
				response->set_errorres("add book comment sus");
			}
			else
			{
				LOG(INFO) <<endl
					  <<"添加书籍评论失败  userId: " <<request->userid()
					  <<" bookId : "<<request->bookid()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
				response->set_errorres("add book comment fail");
			}
		}
		
		virtual void delCommentFun(::google::protobuf::RpcController* control_base,
							const ::BookCommentService::deleteCommentReq* request,
							::commonService::commonResp* response,
							::google::protobuf::Closure* done)
		{//删除评论
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求删除书籍评论 "<<endl;

			//信息判断
			if(request->bookid() == "" || request->userid() < MIN_ACCOUNT)
			{
				LOG(INFO) <<endl
					  <<" 字段错误 bookId :"<<request->bookid()
					  <<" userId "<<request->userid() <<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				return;
			}

			SQL_STATUS ret = __bookCommentSql.delete_comment(request->bookid(),request->userid());
			if(ret == SQL_STATUS::EXE_sus)
			{
				LOG(INFO) <<endl
					  <<"删除评论成功  userId: " <<request->userid()
					  <<" bookId : "<<request->bookid()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus) );
				response->set_errorres("delete book comment sus");
			}
			else
			{
				LOG(INFO) <<endl
					  <<"删除评论失败  userId: " <<request->userid()
					  <<" bookId : "<<request->bookid()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
				response->set_errorres("delete book comment fail");
			}
		}

		virtual void getBookScoreSegStatFun(::google::protobuf::RpcController* control_base,
                       const ::BookCommentService::bookScoreSegStatReq* request,
                       ::BookCommentService::bookScoreSegStatRes* response,
                       ::google::protobuf::Closure* done)
		{//获取评分分段统计信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<" 请求获取书籍评分统计服务 "<<endl;

			//信息判断
			if(request->bookid() == "" || __bookCommentSql.is_existing_by_bookId(request->bookid()) != 1 )
			{
				LOG(INFO) <<endl
					  <<" 获取书籍分段评分统计字段错误 bookId :"
					  <<request->bookid()<<endl;
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
				response->mutable_status()->set_errorres("illegal information");
				return;
			}
			array<int,SCORE_NUM> scoreArr ;
			SQL_STATUS ret = __bookCommentSql.get_score_seg_stat(request->bookid(),scoreArr);
			if(ret == SQL_STATUS::EXE_sus)
			{

				int number ;
				int max_score = SCORE_INTERVAL * SCORE_NUM ;
				for(int score = 1 ,index = 0; score <= max_score ;  score += SCORE_INTERVAL,index++)
				{
					auto seg_stat = response->add_lists();
					seg_stat->set_score(score );
					seg_stat->set_count(scoreArr[index]);
				}

				LOG(INFO) <<endl
					  <<"获取书籍分段评分统计成功  "
					  <<" bookId : "<<request->bookid()
					  <<endl;
				
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				response->mutable_status()->set_errorres("获取书籍分段评分统计成功");
			}
			else 
			{
				LOG(INFO) <<endl
					  <<"获取书籍分段评分统计失败  "
					  <<" bookId : "<<request->bookid()
					  <<endl;
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->mutable_status()->set_errorres("获取书籍分段评分统计失败");
			}
			
			
		}
		
	};

}
