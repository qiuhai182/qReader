#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "SightAnalyze.hpp"
#include "contenttype.hpp"
#include "collectdata.pb.h"
#include "common.pb.h"

using namespace std;
using namespace Analyze;

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
					  << "\n收到请求[log_id=" << control->log_id()
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

		// 获取区间阅读统计数据
		virtual void getReadTimeCount(google::protobuf::RpcController *control_base,
									  const readCountReq *request,
									  readCountResp *response,
									  google::protobuf::Closure *done)
		{
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control =
				static_cast<brpc::Controller *>(control_base);

			LOG(INFO) <<endl
					  << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";

			//一天的专注度统计
			//数据库读取写入csv
			int csvRet = __sightAnalyze.storage_analyse_csv(request->daytime(),request->userid());
			//当天没有数据
			if( csvRet == -1){
				response->mutable_status()->set_code(-1);
				response->mutable_status()->set_errorres("今日未读书");
				LOG(INFO)<<endl
						<< "(客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<<"请求" << request->daytime() << "的阅读分析数据失败)";
				return;
			}
			//当天有数据
			__sightAnalyze.storage_analyse_json(request->userid());
			//结果从json获取
			map<string,string>time_focus ;
			//散点 <action+color,points>
			vector<map<string,vector<map<double,double>>>>scatterDiagram;
			//line Chart
			vector<double>lineChart ;
			int ret = __sightAnalyze.get_analyse_result(time_focus,scatterDiagram,lineChart,request->daytime(),request->userid());

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
				int userId = request->userid();
				string dayTime = request->daytime();
				float intervals[12] = {0};
				for (int i = 0; i < size(intervals); ++i)
				{
					intervals[i] = 0;
				}
				__sightAnalyze.get_interval_count(userId, dayTime, intervals);
				for (int i = 0; i < size(intervals); ++i)
				{
					auto minute = response->add_lists();
					minute->set_min((int)intervals[i]);
				}
				response->mutable_status()->set_code(1);
				LOG(INFO) <<endl
						<< "(客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<<"请求" << request->daytime() << "的阅读分析数据成功)";
			}
			else
			{
				response->mutable_status()->set_code(-1);
				response->mutable_status()->set_errorres("今日未读书");
				LOG(INFO) <<endl
						<< "(客户端ip+port: " << control->remote_side()
						<< " 应答服务器ip+port: " << control->local_side()
						<<"请求" << request->daytime() << "的阅读分析数据失败)";
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
						<< "\n收到请求[log_id=" << control->log_id()
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
						<<"请求阅读计划分钟数)";
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
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
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
