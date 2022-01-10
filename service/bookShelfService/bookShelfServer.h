#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include <tuple>
#include "public/location.hpp"
#include "public/service.hpp"
#include "database/bookShelfSql.hpp"
#include "database/bookSql.hpp"
#include "bookType.hpp"
#include "bitmap.hpp"
#include "contenttype.hpp"
#include "bookshelf.pb.h"
#include "common.pb.h"

using namespace std;
using namespace ormpp;
using namespace service;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(bookShelfPort, 8003, "服务端口");
DEFINE_string(stringPort, "8003", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

static int MIN_ACCOUNT  = 10000 ;

namespace bookShelfService
{
	
	class bookShelfServiceImpl : public bookShelfService
	{ // 个人书架书籍管理服务
	private:
	//填充回发消息
		inline void fillBook(::bookCityService::boocomCombinekInfo*  add_lists,const CombineBook & bookres )
		{
			auto book = add_lists;
			/**
			 * 部分信息需要动态生成
			 */
			//返回数字类型
			int ret_type = bookType::primary_string_to_int(get<0>(bookres).bookType);
			//下载信息
			string bookHeadUrl = FLAGS_bookHeadUrlPre + get<0>(bookres).bookId + ".jpg";
			string bookBodyUrl = FLAGS_bookBodyUrlPre + get<0>(bookres).bookId + ".pdf";
			string filepath = FLAGS_renameBooksPath + get<0>(bookres).bookId + ".pdf";
			struct stat info;
			stat(filepath.c_str(), &info);
			int bookSize = info.st_size;

			//返回填充
			book->set_bookid(get<0>(bookres).bookId);
			book->mutable_baseinfo()->set_bookname(get<0>(bookres).bookName);
			book->mutable_baseinfo()->set_booktype(ret_type);
			book->mutable_baseinfo()->set_authorname(get<0>(bookres).authorName);
			book->mutable_baseinfo()->set_publishtime(get<0>(bookres).publishTime);
			book->mutable_baseinfo()->set_publishhouse(get<0>(bookres).publishHouse);
			book->mutable_baseinfo()->set_bookintro(get<0>(bookres).bookIntro);
			book->mutable_baseinfo()->set_bookpage(get<0>(bookres).bookPage );
			book->mutable_baseinfo()->set_languagetype(get<0>(bookres).languageType );
			book->mutable_downinfo()->set_filesize(bookSize);
			book->mutable_downinfo()->set_bookheadurl(bookHeadUrl);
			book->mutable_downinfo()->set_bookdownurl(bookBodyUrl);
			book->mutable_gradeinfo()->set_remarkcount(get<1>(bookres).count );
			book->mutable_gradeinfo()->set_averagescore(get<1>(bookres).avgScore);

		}
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
	private:
		BookShelfInfoImpl __bookshelfSql;
		BookInfoImpl __bookSql ;
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
			

			//查询结构
			CombineBook book;
			//非法信息处理
			if(request->userid() < MIN_ACCOUNT || request->bookid() == "" || request->addtime() == ""){
				LOG(INFO) << "字段错误，userId : " << request->userid()
						<<" bookId  :  "<<request->bookid()
						<<" addTime :"<<request->addtime()<<endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
				response->set_errorres("参数缺失");
				return ;
			}else{
				
				SQL_STATUS ret = __bookSql.get_book_by_book_id(book,request->bookid());
				if( ret != SQL_STATUS::EXE_sus || get<0>(book).bookName == ""){
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Useless_inf));
					response->set_errorres("书籍不存在");
					LOG(INFO) << "bookId不存在 : "<<request->bookid()<<endl;
					return ;
				}
			}
			UserShelfTable  buffer ;
			buffer.addTime = request->addtime();
			buffer.autoBookId = get<0>(book).autoBookId;
			buffer.bookId	= get<0>(book).bookId;
			buffer.userId = request->userid();
			buffer.isRemove = 0;
			SQL_STATUS ret =  __bookshelfSql.insert_shelf(buffer);
			if (ret !=  SQL_STATUS::EXE_sus)
			{
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("insert failed");
			}else{
                response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
			    response->set_errorres("insert sucessful");
            }
			
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
			
			//非法信息处理
			if(request->userid() < MIN_ACCOUNT || request->bookids().size() == 0){
				LOG(INFO) << "字段缺失，userId : " << request->userid()
						<<" bookIds.size  :  "<<request->bookids().size()<<endl;
				response->set_userid(request->userid());
				response->set_suscount(-1);
				return ;
			}

			int size  = request->bookids_size();
			int sus = 0, fail = 0;
			SQL_STATUS ret ;
			for(int i = 0 ; i < size; i++){
				ret = __bookshelfSql.remove_shelf_book(request->userid(), request->bookids(i));
				if(SQL_STATUS::EXE_sus == ret )
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
			
			//非法信息处理
			if(request->userid() < MIN_ACCOUNT ){
				LOG(INFO) << "字段缺失，userId : " << request->userid()<<endl;
				response->set_count(0);
				return ;
			}
			
			vector<UserShelfTable> shelf_book;
			SQL_STATUS ret;
			ret =   __bookshelfSql.get_book_by_userid(shelf_book, request->userid());
			int size = shelf_book.size();
			if( SQL_STATUS::EXE_sus != ret || size == 0)
			{
				LOG(INFO) << "查询失败 : " << request->userid()<<endl;
				response->set_count(0);
				return ;
			}

			int sus = 0;
			CombineBook buffer ;
			for(auto &  temp:shelf_book)
			{
				ret = __bookSql.get_book_by_autoBookId(buffer,temp.autoBookId);
				if(ret != SQL_STATUS::EXE_sus)
					continue;
				auto book = response->add_lists();
				fillBook(book,buffer);
				sus++ ;

			}

			if ( 0 != sus)
			{
				response->set_count(sus);
				LOG(INFO) << endl
						  << control->remote_side() << "请求获取用户 " 
						  << request->userid() << " 书架的所有书籍成功" 
						  << endl;
			}
			else
			{
				response->set_count(0);
				LOG(INFO) << endl
						  << control->remote_side() 
						  << "请求获取用户 " << request->userid() 
						  << " 书架的所有书籍失败，可能书架为空或用户不存在" 
						  << endl;
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
