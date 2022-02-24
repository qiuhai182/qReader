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
#include "database/bookSql.hpp"
#include "public/service.hpp"
#include "public/location.hpp"
#include "bitmap.hpp"
#include "contenttype.hpp"
#include "bookcity.pb.h"
#include "common.pb.h"
#include "membooks.hpp"
#include "bookType.hpp"

using namespace std;
using namespace ormpp;
using namespace service;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(bookCityport, 8002, "服务端口");
DEFINE_string(stringPort, "8002", "服务端口");
DEFINE_string(ipPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");


#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

 static int MIN_ACCOUNT  = 10000 ;

namespace bookCityService
{
			

	class bookCityServiceImpl : public bookCityService
	{
	private:
		BookInfoImpl __bookCitySql;
		MemBooks __memBookList;			// 实例化书籍，配合模糊匹配
		map<int, int> recommendTimes;	// 记录个性化推荐批次
		map<int, int> browseTimes;		// 记录书城浏览批次
		inline void fillBook(::bookCityService::boocomCombinekInfo*  add_lists,const CombineBook & bookres )
		{ // 填充回发消息
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
			book->mutable_gradeinfo()->set_averagescore(get<1>(bookres).avgScore * 0.1);//浮点回发
		}
		//附加类型填充  附加字段暂时只有bookTitle  后可将第三字段变为位运算值
		inline void fillBook(::bookCityService::boocomCombinekInfo*  add_lists,const CombineBook & bookres ,const int value )
		{
				fillBook(add_lists,bookres);
				if(value == 0)
				{
					add_lists->set_booktitle("快来看看这本<<" + get<0>(bookres).bookName + ">>" );
				}
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
			vector<CombineBook> bookres; 
			if( SQL_STATUS::EXE_sus == __bookCitySql.get_all_book_info(bookres) ){
				if(bookres.size() != 0){
					__memBookList.setBooks(std::move(bookres) );
				}else{	
					std::cerr << "初始化实例书籍时查询数据库结果为空";
				}
			}else{
				std::cerr << "初始化实例书籍时查询数据库失败";
			}
		};
		virtual ~bookCityServiceImpl(){};
		virtual void addBookInfoFun(google::protobuf::RpcController *control_base,
									const bookBaseInfo *request,
									commonService::commonResp *response,
									google::protobuf::Closure *done)
		{ // 添加书籍信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
			/*信息验证*/
			if( request->has_bookid() == false)
			{
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf) );
				response->set_errorres("illegal information");
				LOG(INFO) << control->remote_side() << "传入信息缺少字段" << endl;
			}
			//类型转化
			string bookType = bookType::primary_enum_to_string(static_cast<bookType::primaryClass>(request->booktype()));
			//插入
			SQL_STATUS ret = __bookCitySql.insert_book_info(0,request->bookid(),
														request->bookname(),request->authorname(),
														bookType,request->publishhouse(),
														request->publishtime(),request->bookintro(),
														request->bookpage(),request->languagetype()
													);
			//实例书籍添加 缺省

			if (ret != SQL_STATUS::EXE_sus)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err) );
				response->set_errorres("add faild");
				LOG(INFO)<< control->remote_side() << "导入书籍失败" 
						<< " 错误码 "<<static_cast<int>(ret)
						<<" bookId "<< request->bookid()<<endl;
			}
			else
			{
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
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
		{ // 搜索某本书相关信息  仅单项的搜索
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  << endl;
			SQL_STATUS ret ; 
			if (request->has_bookid())
			{
				CombineBook bookres;
				ret = __bookCitySql.get_book_by_book_id(bookres, request->bookid());
				if (ret == SQL_STATUS::EXE_sus)
				{
					auto book = response->add_lists();
					fillBook(book,bookres);
					if(SQL_STATUS::EXE_sus != __bookCitySql.plus_search_times(
							get<0>(bookres).autoBookId,	get<0>(bookres).bookId,
							request->daytime(),get<0>(bookres).bookName))
						LOG(WARNING) << "更新搜索次数失败" << " bookId "<< get<0>(bookres).bookId << endl;
					response->set_count(1);
				}
				else
					response->set_count(0);
				LOG(INFO) << endl
					<< control->remote_side()
					<< " 搜索图书方式为 bookId "
					<<" 搜索值为  "<<request->bookid() <<endl;
				return ;
			}
			/*选项时判断*/
			//信息判断
			if(request->userid() < MIN_ACCOUNT || request->offset() < 0 || request->count() < 0)
			{
				response->set_count(0);
					LOG(INFO) << endl
							<< control->remote_side()
							<< " 字段错误  userId :"
							<<request->userid()
							<<" offset "<<request->offset()
							<<" count "<<request->count()<< endl;
					return;
			}
			/**
			 * 获取选项，根据选项查找
			 */
			string optionName ,optionValue ;
			if (request->has_bookname())
			{
				optionName = "bookName";
				optionValue = request->bookname();
			}
			if (request->has_authorname())
			{
				optionName = "authorName";
				optionValue = request->authorname();
			}
			if (request->has_publishhouse())
			{
				optionName = "publishHouse";
				optionValue = request->publishhouse();
			}
			if (request->has_booktype())
			{
				if(  false == bookType::isPrimaryClass(request->booktype())){
					response->set_count(0);
					LOG(INFO) << endl
							<< control->remote_side()
							<< "搜索错误类型图书" << endl;
					return;
				}
				optionName = "bookType";
				//变为枚举
				bookType::primaryClass typeEnum = static_cast<bookType::primaryClass>(request->booktype());
				optionValue = bookType::primary_enum_to_string(typeEnum) ;
			}
			LOG(INFO) << endl
					<< control->remote_side()
					<< " 搜索图书方式为 " << optionName
					<<" 搜索值为  "<<optionValue <<endl;

			vector<CombineBook> bookres;
			ret =  __bookCitySql.get_books_by_option(bookres,optionName,optionValue,request->offset(),request->count());
			for (int i = 0; i < bookres.size(); ++i)
			{
				auto book = response->add_lists();
				fillBook(book,bookres[i]);
				if(SQL_STATUS::EXE_sus != __bookCitySql.plus_search_times(
						get<0>(bookres[i]).autoBookId,	get<0>(bookres[i]).bookId,
						request->daytime(),get<0>(bookres[i]).bookName 
					)
				)
				{
					LOG(WARNING) << "更新搜索次数失败"<<endl ;
				}
			}
			response->set_count(bookres.size());

			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void setBookInfoFun(google::protobuf::RpcController *control_base,
									const updateBookBaseInfo *request,
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

			map<option,sqlUpdateVal> up_data;
			sqlUpdateVal buffer;
			//必填字段
			if(request->has_bookid() == true)
			{
				if(request->bookid() != "")
				{
					buffer.strVal = request->bookid();
					buffer.type = COND_TYPE::String;
					up_data["bookId"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->bookid()<<endl;
					return ;
				}
			}
			//更改选项
			if(request->has_bookname() == true)
			{
				if(request->bookname() != "")
				{
					buffer.strVal = request->bookname();
					buffer.type = COND_TYPE::String;
					up_data["bookName"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->bookname()<<endl;
					return ;
				}
			}
			if(request->has_authorname() == true)
			{
				if(request->authorname() != "")
				{
					buffer.strVal = request->authorname();
					buffer.type = COND_TYPE::String;
					up_data["authorName"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->authorname()<<endl;
					return ;
				}
			}
			if(request->has_booktype() == true)
			{
				if(bookType::isPrimaryClass(request->booktype()))
				{
					buffer.strVal = request->booktype();
					buffer.type = COND_TYPE::Int;
					up_data["bookType"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->booktype()<<endl;
					return ;
				}
			}
			if(request->has_publishhouse() == true)
			{
				if(request->publishhouse() != "")
				{
					buffer.strVal = request->publishhouse();
					buffer.type = COND_TYPE::String;
					up_data["publishHouse"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->publishhouse()<<endl;
					return ;
				}
			}
			if(request->has_publishtime() == true)
			{
				if(request->publishtime() != "")
				{
					buffer.strVal = request->publishtime();
					buffer.type = COND_TYPE::String;
					up_data["publishTime"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->publishtime()<<endl;
					return ;
				}
			}
			if(request->has_bookpage() == true)
			{
				if(request->bookpage() > 0)
				{
					buffer.strVal = request->bookpage();
					buffer.type = COND_TYPE::Int;
					up_data["bookPage"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->bookpage()<<endl;
					return ;
				}
			}
			if(request->has_languagetype() == true)
			{
				if(request->languagetype() > 0)
				{
					buffer.strVal = request->languagetype();
					buffer.type = COND_TYPE::Int;
					up_data["languageType"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->languagetype()<<endl;
					return ;
				}
			}
			if(request->has_bookintro() == true)
			{
				if(request->bookintro() != "")
				{
					buffer.strVal = request->bookintro();
					buffer.type = COND_TYPE::String;
					up_data["bookIntro"] = buffer;
				}
				else
				{
					response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
					response->set_errorres("Fields are missing");
					LOG(INFO) << endl
							<< control->remote_side() << "  缺少字段  "
							 << request->bookintro()<<endl;
					return ;
				}
			}
			//进行修改
			SQL_STATUS ret =   __bookCitySql.up_book_Info(up_data);
			if (ret != SQL_STATUS::EXE_sus)
			{
				cout << endl;
				LOG(INFO) << "[" << __FILE__ << "]"
						  << "[" << __LINE__ << "]" << endl;
				response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->set_errorres("update faild");
				LOG(INFO) << endl
                        << control->remote_side() << "  更新书籍信息失败" << endl;
			}
            else
            {
                response->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
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
									   const offsetCountBooksReq *request,
									   booksRespList *response,
									   google::protobuf::Closure *done)
		{ // 获取数据库所有书籍信息
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
					
			vector<CombineBook> bookres;
			SQL_STATUS ret =  __bookCitySql.get_all_book_info(bookres);
			
			if(SQL_STATUS::EXE_sus == ret )
			{
				int size = bookres.size();
				for (int i = 0; i < size; ++i)
				{
					auto book = response->add_lists();
					fillBook(book,bookres[i]);
				}
				response->set_count(size);

				LOG(INFO) << endl
						<< control->remote_side() 
						<< " 查询所有书籍成功，本次查到 " 
						<< size << " 本书 " << endl;
			}
			else
			{
				response->set_count(0);

				LOG(INFO) << endl
						<< control->remote_side() 
						<< " 查询所有书籍失败 "<<endl;
			}
			
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
			//缺省，包含多表约束的删除和实例化书籍删除
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

			vector<CombineBook> bookres;
			SQL_STATUS ret =  __bookCitySql.get_recommend_book(bookres, curIndex);
			if (ret != SQL_STATUS::EXE_sus)
			{
				response->set_count(0);
				LOG(INFO) <<endl
						<< "[" << __FILE__ << "]"
						<< "[" << __LINE__ << "]" 
						<< " 查询个性化推荐失败 "<<endl;
			}
			else
			{
				int start = 0 ,size = bookres.size();
				//int i = start;
				int end = std::min(start + 10, size);
				for (; start < end; ++start)
				{
					auto book = response->add_lists();
					fillBook(book,bookres[start]);
				}
				response->set_count(size);
				LOG(INFO) << endl
						<< control->remote_side() << "查询个性化推荐成功，本次查到 " 
						<< size << " 本书" << endl;
			}
			
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
			vector<CombineBook> bookres;
			SQL_STATUS ret =  __bookCitySql.get_recommend_book(bookres, curIndex);
			if (ret != SQL_STATUS::EXE_sus)
			{
				LOG(INFO)<< endl
						<< "[" << __FILE__ << "]"
						<< "[" << __LINE__ << "]" 
						<< " 浏览推荐失败 "<<endl;
				response->set_count(0);
			}
			else
			{
				int start = 0 ,size = bookres.size();
				//int i = start;
				int end = std::min(start + 10, size);
				for (; start < end; ++start)
				{
					auto book = response->add_lists();
					fillBook(book,bookres[start]);
				}
				response->set_count(size);
				LOG(INFO) << endl
						<< control->remote_side() 
						<< "查询随机书城推荐成功，本次查到 " 
						<< size << " 本书" << endl;
			}
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}

		virtual void getMostlySearchFun(google::protobuf::RpcController *control_base,
                       const ::bookCityService::universalBlankReq* request,
                       ::bookCityService::mostlySearchRes* response,
                       ::google::protobuf::Closure* done)
		{ // 搜索书籍推荐
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << "请求热搜书籍 "
					  << request->count() <<" 本 "
					  <<control->request_attachment() << ")"
					  <<endl;
			if(!request->has_daytime()){
				response->set_count(0);
				LOG(INFO) << endl
					<< control->remote_side() 
					<<" : 查询热搜书籍推荐失败，daytime 字段缺失 "
					<< endl;
				return ;
			}
			vector<SearchStatisticsTable> books ;
			string month = request->daytime().substr(0,7);
			int searchCount = request->count()  ;//请求回发书籍
			// 桶排序查找12月热搜去重  bookid-SearchStatisticsTable
			map<string,SearchStatisticsTable> resultBook ;
			SearchStatisticsTable book;
			for(int mon = 0; mon < 12 ; mon++){
				if(resultBook.size() == 10 ) break; //查找足够提前退出
				SQL_STATUS ret = __bookCitySql.get_mostly_search_by_month_count(month,books,searchCount) ;
				int size = books.size();
				if(SQL_STATUS::EXE_sus != ret)
				{
					LOG(INFO) <<"查询 "<<month<<" 月热搜书籍推荐失败"<< endl;
				}
				else
				{
					for (int start = 0; start < size; ++start){
						auto iter = resultBook.find(books[start].bookId);
						if(iter == resultBook.end()){
							book.bookId = books[start].bookId;
							book.times = books[start].times ;
							book.bookName = books[start].bookName ;
							resultBook.insert(pair<string,SearchStatisticsTable>(book.bookId,book));
						}
					}
				}
				// 月份前移
				reduce_months(month);
			}
			if(resultBook.size() == searchCount){
				// --结果发送  
				for (auto & bookres:resultBook){
					auto book = response->add_lists();
					book->set_bookid(bookres.second.bookId);
					book->set_bookname(bookres.second.bookName);
					book->set_searchtimes(bookres.second.times);
				}
				response->set_count(resultBook.size());
				LOG(INFO) << endl
					<< control->remote_side() <<month 
					<<"月查询热搜书籍推荐成功,共"
					<<resultBook.size()<<"本"<< endl;
			}
			else // 查询不足在书城发送查找发送 包含书籍不足searchCount
			{
				vector<CombineBook>  books;
				SQL_STATUS ret = __bookCitySql.get_book_offset(books,2,searchCount);
				int size = books.size();
				cout<<"size is "<<size <<endl ;
				for(auto & bookres : books){
					auto book = response->add_lists();
					book->set_bookid(get<0>(bookres).bookId);
					book->set_bookname(get<0>(bookres).bookName);
					book->set_searchtimes(1);
				}
				response->set_count(size);
				LOG(INFO) << endl
					<< control->remote_side() << "  "
					<<month <<"月查询热搜书籍推荐不足，书城发送共"
					<<size<<"本"<< endl;
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
				response->set_count(0);
				LOG(WARNING) << endl
					  << control->remote_side() 
					  << "请求数量错误，榜单查询失败" 
					  << endl;
			}


			vector<CombineBook> books ;
			SQL_STATUS ret =  __bookCitySql.get_book_offset(books,0,request->count());

			if( ret != SQL_STATUS::EXE_sus)
			{
				response->set_count(0);
				LOG(WARNING) << endl
					  << control->remote_side() 
					  << " 数据库执行错误，榜单查询失败" 
					  << endl;
			}
			else
			{
				for(auto & bookres:books ){
					auto book = response->add_lists();
					fillBook(book,bookres);
				}
				response->set_count(books.size());
				LOG(WARNING) << endl
					  << control->remote_side() 
					  << " 榜单查询成功,共" 
					  << books.size()<<"本"<<endl;
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
			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ") "
					  <<" 请求模糊匹配书籍 "<<endl;
			//非法信息处理
			if(request->daytime() == "" || request->words() == "" ||
					request->count() <= 0 || request->offset() < 0){
				LOG(INFO) << "字段缺失，或数据非法 daytime : " << request->daytime()
						<<" word  :  "<<request->words()<<" count :" << request->count()
						<<" offset : " << request->offset()<<endl;
				response->set_count(0);
				return ;
			}
			vector<int> autoBookIds;
			//从实例读出书籍
			__memBookList.fuzzySearch(autoBookIds,request->words(),request->offset(),request->count()) ;
			CombineBook bookbuffer ;
			int beginSize = autoBookIds.size(),count =0;
			SQL_STATUS ret ;
			for(int index = 0; index < beginSize; index++)
			{
				ret =__bookCitySql.get_book_by_autoBookId(bookbuffer,autoBookIds[index]);
				if(SQL_STATUS::EXE_sus != ret )
				{
					LOG(INFO)<< endl
						 	<<" 书籍模糊匹配后查找失败 "
							<<" autobookid  "<<autoBookIds[index]
							<<endl;
					continue;
				}
				count++;
				auto book = response->add_lists();
				fillBook(book,bookbuffer);
			}
			cout<<endl <<" 初始值  "<<beginSize<<" 终值  "<<count<<endl ;
			if (count == 0)
			{
				response->set_count(0);
				LOG(INFO) << endl
						  << control->remote_side()
						  << " :未搜索到图书" << endl;
			}
			else
			{
				response->set_count(count);
				LOG(INFO) << endl
						  << control->remote_side()
						  << " :搜索到图书 " << beginSize << " 本。" << endl;
			}

		}
		
<<<<<<< HEAD
		virtual void getPushBooksFun(::google::protobuf::RpcController* control_base,
                       const ::bookCityService::offsetCountBooksReq* request,
                       ::bookCityService::booksRespList* response,
                       ::google::protobuf::Closure* done)
		{//书籍推送
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ") "
					  <<" 请求获取推送书籍 "<<endl;

			//非法信息处理
			if(request->size() <= 0 || request->offset() < 0)
			{
				LOG(INFO) << "字段缺失，或数据非法 "
						<<" count :" << request->size()
						<<" offset : " << request->offset()<<endl;
				response->set_count(0);
				return ;
			}
			vector<CombineBook> books ;
			SQL_STATUS ret =  __bookCitySql.get_book_offset(books,0,request->size());

			if( ret != SQL_STATUS::EXE_sus)
			{
				response->set_count(0);
				LOG(WARNING) << endl
					  << control->remote_side() 
					  << " 数据库执行错误，获取推送失败" 
					  << endl;
			}
			else
			{
				for(auto & bookres:books ){
					auto book = response->add_lists();
					fillBook(book,bookres,0);
				}
				response->set_count(books.size());
				LOG(WARNING) << endl
					  << control->remote_side() 
					  << " 获取推送成功,共" 
					  << books.size()<<"本"<<endl;
			}

		}
	}; 
=======

		
	 	virtual void getTypedBooksFun(::google::protobuf::RpcController* control_base,
										const ::bookCityService::getTypedBookReq* request,
										::bookCityService::booksRespList* response,
										::google::protobuf::Closure* done)
		{ // 根据书籍类型获取指定类别、指定批次书籍
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			LOG(INFO) << endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ") " << endl;
			int offset = 0;
			int count = 10;
			if (request->has_offset())
				offset = request->offset();
			if (request->has_count())
				count = request->count();
			string optionName = "bookType";
			// int转自定义枚举类型
			bookType::primaryClass typeEnum = static_cast<bookType::primaryClass>(request->booktype());
			// 枚举类型转字符串
			string optionValue = bookType::primary_enum_to_string(typeEnum) ;
			vector<CombineBook> bookres;
			SQL_STATUS ret =  __bookCitySql.get_books_by_option(bookres, optionName, optionValue, offset, count);
			for (int i = 0; i < bookres.size(); ++i)
			{
				auto book = response->add_lists();
				fillBook(book, bookres[i]);
			}
			response->set_count(bookres.size());
			LOG(INFO) << endl
						<< control->remote_side()
						<< "浏览书籍类型:" << optionValue << ",偏移值:" << offset << "," << bookres.size() << " 本" << endl;
		}

	};
>>>>>>> qiuhai

}

