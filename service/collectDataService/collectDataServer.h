#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "public/service.hpp"
#include "SightAnalyze.hpp"
#include "contenttype.hpp"
#include "collectdata.pb.h"
#include "common.pb.h"

using namespace std;
using namespace Analyze;
using namespace service;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(collectDataPort, 8004, "服务端口");
DEFINE_string(stringPort, "8004", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

namespace collectdataService
{

	class collectDataServiceImpl : public collectService
	{
	private :
		SightAnalyze   __sightAnalyze;
		map<int, int> userTarget;

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

			LOG(INFO) <<endl
					  << "\n收到 请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")"
					  <<endl;
			string bookId = request->bookid();
			int userId = request->userid();
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
			SQL_STATUS ret = __sightAnalyze.insert_sight_data(bufSight);
			if (ret == SQL_STATUS::EXE_sus)
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

		// 获取阅读分析
		virtual void getReadSightAnalyResFun(google::protobuf::RpcController *control_base,
									  const readCountReq *request,
									  readSightAnalyeReq *response,
									  google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到 请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";

			//信息判断
			if(request->daytime() == "")
			{//应当添加用户判断 
				LOG(INFO) <<endl
					  <<"  请求视线分析结果 字段错误 dayTime :"<<request->daytime()
					  <<" userId :"<<request->userid() 
					  <<endl;
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Illegal_inf));
				response->mutable_status()->set_errorres("illegal information");
				return;
			}

			//一天的专注度统计
			//数据库读取写入csv
			// int csvRet = __sightAnalyze.storage_analyse_csv(request->daytime(),request->userid());
			// //当天没有数据
			// if( csvRet == -1){
			// 	response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Useless_inf));
			// 	response->mutable_status()->set_errorres("今日未读书");
			// 	LOG(INFO)<<endl
			// 			<< "dayTime :"<<request->daytime()
			// 		  	<<" userId "<<request->userid()
			// 			<<" 请求" << request->daytime() << "的阅读分析数据失败,无数据";
			// 	return;
			// }
			//当天有数据
			__sightAnalyze.storage_analyse_json(request->userid());
			//结果从json获取
			readAnalyzeRes res;

			//12时段
			int userId = request->userid();
			string dayTime = request->daytime();
			float intervals[12];
			for (int i = 0; i < size(intervals); ++i)
			{
				intervals[i] = -1;
			}
				__sightAnalyze.get_interval_count(userId, dayTime, intervals);
			if(intervals[11] == -1)
			{
				LOG(INFO)<<endl
					<< "dayTime :"<<request->daytime()
					<<" userId "<<request->userid()
					<<"  请求" << request->daytime() << "的阅读分析数据失败,时段获取错误)";
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->mutable_status()->set_errorres("时段获取错误");
				return;
			}
			

			bool ret = __sightAnalyze.get_analyse_result(res,request->daytime(),request->userid());

			if(ret == true && res.isCorrect())//是否有正确结果
			{
				//将赋值放入最后统一，保证不出现多余值
				for (int i = 0; i < size(intervals); ++i)
				{
					response->add_timelists((int)intervals[i]);
				}

				response->set_hour(res.IntRes["hour"]);
				response->set_min(res.IntRes["min"]);
				response->set_sec(res.IntRes["sec"]);
				response->set_pages(res.IntRes["pages"]);
				response->set_rows(res.IntRes["rows"]);
				response->set_focus(res.focus);

				for(auto item:res.speedPoint)
				{
					response->add_speedpoints(item) ;
				}
				for(auto item:res.chart)
				{
					auto chart = response->add_pipchart();
					chart->set_behavior(item.behavior);
					chart->set_percentage(item.Percentage);
				}

				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Sus));
				LOG(INFO)<<endl
						<< "dayTime :"<<request->daytime()
					  	<<" userId "<<request->userid()
						<<"  请求" << request->daytime() << "的阅读分析数据成功)";
			}
			else
			{
				response->mutable_status()->set_code(static_cast<int>(SERVICE_RET_CODE::SERVICE_Err));
				response->mutable_status()->set_errorres("今日未读书");
				LOG(INFO) <<endl
						<< "dayTime : :"<<request->daytime()
					  	<<" userId "<<request->userid()
						<<"  请求" << request->daytime() << "的阅读分析数据失败,读取失败)";
			}

			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
		
		// 获取目标时间
		virtual void getTargetMinuteFun(google::protobuf::RpcController *control_base,
									  const getTargetMinuteReq *request,
									  targetMinute *response,
									  google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			LOG(INFO) 	<<endl
						<< "\n收到 请求[log_id=" << control->log_id()
						<< "] 客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<< " (attached : " << control->request_attachment() << ")";
			int userId = request->userid();
			if (userTarget.count(userId))
			{
				response->set_minute(userTarget[userId]);
			}
			else
				response->set_minute(0);
			LOG(INFO) 	<<endl
						<< "(客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<<" 请求阅读计划分钟数)";
		}

		// 设置目标时间
		virtual void setTargetMinuteFun(google::protobuf::RpcController *control_base,
										const setTargetMinuteReq *request,
										commonService::commonResp *response,
										google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到 请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			int userId = request->userid();
			userTarget[userId] = request->minute();
			response->set_code(1);
			response->set_errorres("成功");
			LOG(INFO) 	<<endl
						<< "(客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<< "设置阅读计划分钟数)";
		}

		
	};

}
