/*
 * @Author: zqj
 * @Date: 2021-11-06 00:22:30
 * @LastEditTime: 2021-11-07 00:15:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /qReader_o/bookShelfService/bookShelfServer.h
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
#include "bookshelf.pb.h"
#include "common.pb.h"

using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(bookShelfPort, 8003, "服务端口");
DEFINE_string(stringPort, "8003", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

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
