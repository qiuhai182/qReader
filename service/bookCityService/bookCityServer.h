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
#include "bookcity.pb.h"
#include "common.pb.h"
#include "membooks.hpp"

using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(bookCityport, 8002, "服务端口");
DEFINE_string(stringPort, "8002", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

namespace bookCityService
{
			

	class bookCityServiceImpl : public bookCityService
	{
	private:
		MemBooks m_memBookList ;				// 实例化书籍，配合模糊匹配
		map<string, int> recommendTimes;	// 记录个性化推荐批次
		map<string, int> browseTimes;		// 记录书城浏览批次
	private:
		//暂时用原表结果其他位填充
		inline void fillBook(::bookCityService::boocomCombinekInfo*  add_lists,const BookInfoTable & bookres )
		{
			auto book = add_lists;

			book->set_bookid(bookres.bookId);
			book->mutable_baseinfo()->set_bookname(bookres.bookName);
			book->mutable_baseinfo()->set_booktype(1000);
			book->mutable_baseinfo()->set_authorname(bookres.authorName);
			book->mutable_baseinfo()->set_publishtime(bookres.publishTime);
			book->mutable_baseinfo()->set_publishhouse("机械工业出版社");
			book->mutable_baseinfo()->set_bookintro(bookres.bookIntro);
			book->mutable_baseinfo()->set_bookpage(200);
			book->mutable_baseinfo()->set_languagetype(2);
			book->mutable_downinfo()->set_filesize(20.3);
			book->mutable_downinfo()->set_bookheadurl(bookres.bookHeadUrl);
			book->mutable_downinfo()->set_bookdownurl(bookres.bookDownUrl);
			book->mutable_gradeinfo()->set_remarkcount(100);
			book->mutable_gradeinfo()->set_averagescore(200.4);

		}
		void reduce_months(string  & monthTime)
		{//减小月份xxxx-xx

			//确定提取位
			int subrCount = monthTime[5] == '0' ? 6 :5 ;
			string month = monthTime.substr(subrCount);
			int monthInt = atoi(month.c_str()) ;
			if( monthInt == 1)
			{//更改年份
				int yearInt = atoi(monthTime.substr(0,4).c_str())  - 1;
				monthTime = to_string(yearInt) + "-12" ;
			}
			else
			{
				string zero =  monthInt < 10 ? "0":"" ; //是否填充0
				monthTime = monthTime.substr(0,5) + zero  + to_string(monthInt - 1) ;
			}
		}
	public:
		bookCityServiceImpl(){
			vector<BookInfoTable> bookres; 
			if( 1 != get_all_book(bookres) ){
				if(bookres.size() != 0){
					m_memBookList.setBooks(std::move(bookres) );
				}else{	
					std::cerr << "初始化实例书籍时查询数据库结果为空";
				}
			}else{
				std::cerr << "初始化实例书籍时查询数据库失败";
			}
		};
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
			int ret = insert_book(request->bookid(), request->bookname(),
								request->bookheadurl(),request->bookdownurl(),
								request->authorname(),request->booktype(),
								request->bookintro(),request->publishtime());
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
					fillBook(book,bookres);
					if(-1 == plus_search_times(bookres.bookId,request->daytime(),bookres.bookName) ){
						LOG(WARNING) << "更新搜索次数失败"<<endl ;
					}
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
					fillBook(book,bookres[i]);
					if(-1 == plus_search_times(bookres[i].bookId,request->daytime(),bookres[i].bookName) ){
						LOG(WARNING) << "更新搜索次数失败"<<endl ;
					}
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
					fillBook(book,bookres[i]);
					if(-1 == plus_search_times(bookres[i].bookId,request->daytime(),bookres[i].bookName )){
						LOG(WARNING) << "更新搜索次数失败"<<endl ;
					}
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
					  << " (attached : " << control->request_attachment() << ")"
					  <<"请求修改书籍信息";

			//非法信息处理
			if(!request->has_authorname() && !request->has_bookdownurl()
				&& !request->has_bookheadurl() && !request->has_bookname() 
				&& !request->has_booktype() && !request->has_hits())
			{
				response->set_code(-1);
				response->set_errorres("Fields are missing");
				LOG(INFO) << endl
                        << control->remote_side() << "   缺少字段" << endl;
				return ;
			}

			int ret = up_book(request->bookid(), request->bookname(),
							request->bookheadurl(), request->bookdownurl(), 
							request->booktype(), request->authorname(),
							request->bookintro(),request->publishtime());
			if (ret == 0)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(-1);
				response->set_errorres("update faild");
				LOG(INFO) << endl
                        << control->remote_side() << "  更新书籍信息失败" << endl;
			}
			else if (ret == -1)//空串
			{
				response->set_code(-1);
                response->set_errorres("block Fields");
                LOG(INFO) << endl
                        << control->remote_side() << "  填入了空的字段" << endl;
			}
            else
            {
                response->set_code(1);
                response->set_errorres("sucessful");
                LOG(INFO) << endl
                        << control->remote_side() << "更新书籍信息成功" << endl;
            }
			
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
			vector<BookInfoTable> bookres;
			int ret = get_all_book(bookres); // 获取所有书籍信息
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
				fillBook(book,bookres[start]);
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
				response->set_errorres("delete faild");
			}
            else
            {
                response->set_code(1);
                response->set_errorres("delete sucessful");
                LOG(INFO) << endl
                        << control->remote_side() << "删除书籍成功" << endl;
            }
			
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getBookADSFun(google::protobuf::RpcController *control_base,
									   const adsReq *request,
									   adsRes *response,
									   google::protobuf::Closure *done)
		{ // 获取广告图片
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			vector<BookADSTable> ads;
			int ret = get_all_ads(ads);
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
			}
			int start = 0;
			int i = start;
			int end = std::min(start + 10, ret);
			int count = 0;
			for (; start < ret; ++start)
			{
				auto adRes = response->add_lists();
				adRes->set_adurl(ads[start].adUrl);
				BookInfoTable bookres;
				int retBook = get_book_by_id(bookres, ads[start].bookId);
				if (retBook != -1)
				{
					auto adBookRes = adRes->add_lists();
					fillBook(adBookRes,bookres);
				}
				++count;
			}
			response->set_count(count);
			LOG(INFO) << endl
					  << control->remote_side() << "查询广告成功" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getRecommendBookFun(google::protobuf::RpcController *control_base,
									   const recommendBookReq *request,
									   recommendBookRes *response,
									   google::protobuf::Closure *done)
		{ // 获取个性化书籍推荐
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int curIndex = 0;
			if (recommendTimes.count(request->userid()))
			{
				curIndex = recommendTimes[request->userid()] + 1;
				recommendTimes[request->userid()] = curIndex;
			}
			else
			{
				recommendTimes[request->userid()] = curIndex;
			}
			vector<BookInfoTable> books;
			int ret = get_recommend_book(books, curIndex);
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
			}
			int start = 0;
			int i = start;
			int end = std::min(start + 10, ret);
			for (; start < ret; ++start)
			{
				auto book = response->add_lists();
				fillBook(book,books[start]);
			}
			response->set_count(ret);
			LOG(INFO) << endl
					  << control->remote_side() << "查询个性化推荐成功，本次查到 " << ret << " 本书" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
		
		virtual void getBrowseBookFun(google::protobuf::RpcController *control_base,
									   const browseBookReq *request,
									   browseBookRes *response,
									   google::protobuf::Closure *done)
		{ // 获取书城浏览推荐
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int curIndex = 0;
			if (browseTimes.count(request->userid()))
			{
				curIndex = browseTimes[request->userid()] + 1;
				browseTimes[request->userid()] = curIndex;
			}
			else
			{
				browseTimes[request->userid()] = curIndex;
			}
			vector<BookInfoTable> books;
			int ret = get_recommend_book(books, curIndex);
			if (ret == -1)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
			}
			int start = 0;
			int i = start;
			int end = std::min(start + 10, ret);
			for (; start < ret; ++start)
			{
				auto book = response->add_lists();
				fillBook(book,books[start]);
			}
			response->set_count(ret);
			LOG(INFO) << endl
					  << control->remote_side() << "查询随机书城推荐成功，本次查到 " << ret << " 本书" << endl;
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
		virtual void getMostlySearchFun(google::protobuf::RpcController *control_base,
                       const ::bookCityService::universalBlankReq* request,
                       ::bookCityService::mostlySearchRes* response,
                       ::google::protobuf::Closure* done)
		{//搜索书籍推荐
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << "请求热搜书籍 "
					  << request->count() <<" 本 "
					  <<control->request_attachment() << ")";

			if(!request->has_daytime()){
				response->set_count(-2);
				LOG(INFO) << endl
					<< control->remote_side() <<"查询热搜书籍推荐失败，daytime 字段缺失 "
					<< endl;
				return ;
			}
			vector<SearchStatisticsTable> books ;
			string month = request->daytime().substr(0,7);
			int searchCount = request->count()  ;//请求回发书籍
			//桶排序查找12月热搜去重  bookid-SearchStatisticsTable
			map<string,SearchStatisticsTable> resultBook ;
			SearchStatisticsTable book;

			for(int mon = 0;mon < 12 ; mon++){
				if(  resultBook.size() == 10 ) break ;//查找足够提前退出
				int ret = get_mostly_search_by_month_count(month,books,searchCount) ;
				if(-1 == ret)
				{
					LOG(INFO) <<"查询 "<<month<<" 月热搜书籍推荐失败"<< endl;
				}
				else
				{
					for (int start = 0; start < ret; ++start){
						auto iter = resultBook.find(books[start].bookId);
						if(iter == resultBook.end()){
							book.bookId = books[start].bookId;
							book.times = books[start].times ;
							book.bookName = books[start].bookName ;
							resultBook.insert(pair<string,SearchStatisticsTable>(book.bookId,book));
						}
					}	
					
				}
				//月份前移
				reduce_months(month);
			}
			
			if(resultBook.size() == searchCount){
				//--结果发送  
				for (auto & bookres:resultBook){
					auto book = response->add_lists();
					book->set_bookid(bookres.second.bookId);
					book->set_bookname(bookres.second.bookName);
					book->set_searchtimes(bookres.second.times);
				}
				response->set_count(resultBook.size());
				LOG(INFO) << endl
					<< control->remote_side() <<month <<"月查询热搜书籍推荐成功,共"
					<<resultBook.size()<<"本"<< endl;
			}
			else//查询不足在书城发送查找发送 包含书籍不足searchCount
			{
				vector<BookInfoTable>  books;
				int ret = get_book_offset(books,2,searchCount);
				cout<<"ret is "<<ret <<endl ;
				for(auto & bookres : books){
					auto book = response->add_lists();
					book->set_bookid(bookres.bookId);
					book->set_bookname(bookres.bookName);
					book->set_searchtimes(1);
				}
				response->set_count(ret);
				LOG(INFO) << endl
					<< control->remote_side() << "  "
					<<month <<"月查询热搜书籍推荐不足，书城发送共"
					<<ret<<"本"<< endl;
			}
			
		}
		virtual void getPopularSearchFun(google::protobuf::RpcController *control_base,
							const ::bookCityService::universalBlankReq* request,
							::bookCityService::booksRespList* response,
							::google::protobuf::Closure* done)
		{//榜单推荐
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			//非法数据处理
			if(request->count() <= 0)
			{
				response->set_count(-1);
				LOG(WARNING) << endl
					  << control->remote_side() << "榜单查询失败,qong" << endl;
			}


			vector<BookInfoTable> books ;
			int ret = get_book_offset(books,1,request->count());

			if( ret == -1)
			{
				response->set_count(-1);
				LOG(WARNING) << endl
					  << control->remote_side() << "榜单查询失败" << endl;
			}
			else
			{
				for(auto & bookres:books ){
					auto book = response->add_lists();
					fillBook(book,bookres);
				}
				response->set_count(ret);
				LOG(WARNING) << endl
					  << control->remote_side() << "榜单查询成功,共" << ret<<"本"<<endl;
			}

		}

		
	 	virtual void FuzzySearchBooksFun(::google::protobuf::RpcController* control_base,
										const ::bookCityService::fuzzySearchRequest* request,
										::bookCityService::booksRespList* response,
										::google::protobuf::Closure* done)
		{//模糊匹配书籍
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ") "
					  <<"请求模糊匹配书籍";

			//非法信息处理
			if(request->daytime() == "" || request->words() == "" ||
					request->count() <= 0 || request->offset() < 0){
				LOG(INFO) << "字段缺失，或数据非法 daytime : " << request->daytime()
						<<" word  :  "<<request->words()<<" coutn :" << request->count()
						<<" offset : " << request->offset()<<endl;
				response->set_count(-1);
				return ;
			}

			vector<BookInfoTable> books;
			//从实例读出书籍
			m_memBookList.fuzzySearch(books,request->words(),request->offset(),request->count()) ;
			cout<<"list size is "<<m_memBookList.size()<<endl ;
			int size = books.size();
			response->set_count(size);
			for (int i = 0; i < size ; ++i)
			{
				auto book = response->add_lists();
				fillBook(book,books[i]);
				if(-1 == plus_search_times(books[i].bookId,request->daytime(),books[i].bookName) ){
					LOG(WARNING) << " : 更新搜索次数失败"<<endl ;
				}
			}
			if (size == 0)
			{
				response->set_count(-1);
				LOG(INFO) << endl
						  << control->remote_side()
						  << " :未搜索到图书" << endl;
			}
			else
			{
				
				LOG(INFO) << endl
						  << control->remote_side()
						  << "搜索到图书" << size << " 本。" << endl;
			}

		}
	}; 

}