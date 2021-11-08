/*
 * @Author: zqj
 * @Date: 2021-11-06 00:34:56
 * @LastEditTime: 2021-11-07 00:58:15
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /qReader/commonService/commonServer.h
 */
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
#include "common.pb.h"

using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(commonPort, 8005, "服务端口");
DEFINE_string(stringPort, "8005", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小


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
