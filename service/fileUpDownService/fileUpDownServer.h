#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <brpc/server.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "public/location.hpp"
#include "bitmap.hpp"
#include "contenttype.hpp"
#include "fileupdown.pb.h"
#include "database/bookSql.hpp"
using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(fileUpDownPort, 8006, "服务端口");
DEFINE_string(stringPort, "8006", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小


namespace fileService
{

	class fileServiceImpl : public fileService
	{ // 文件传输服务
		BookInfoImpl __bookSql;
	public:
		fileServiceImpl(){};
		virtual ~fileServiceImpl(){};

		void sendFile(brpc::Controller *control, string filePath, string contentType)
		{ // 发送文件函数
			std::fstream tmpfile;
			tmpfile.open(filePath, ios::in | ios::binary | ios::ate); // 二进制输入(读取),定位到文件末尾
			if (tmpfile.is_open())
			{
				// 打开文件成功，contentType不为空
				control->http_response().set_content_type(static_cast<char *>(contentType.data()));
				size_t length = tmpfile.tellg(); // 获取文件大小
				int all_len = length;
				tmpfile.seekg(0, std::ios::beg); // 定位到流开始
				butil::IOBuf os;
				char data[IOBuf_MAX_SIZE];
				while (length >= IOBuf_MAX_SIZE)
				{
					os.clear(); // 清空流
					tmpfile.read(data, IOBuf_MAX_SIZE);
					os.append(data, IOBuf_MAX_SIZE);
					control->response_attachment().append(os);
					length -= IOBuf_MAX_SIZE;
				}
				if (length > 0)
				{
					os.clear(); // 清空流
					tmpfile.read(data, length);
					os.append(data, length);
					control->response_attachment().append(os);
					length = 0;
				}
				LOG(INFO) << "请求下载文件: " << filePath << " 成功" << endl;
				control->http_response().set_status_code(200);
			}
			else
			{ // 读取失败
				LOG(INFO) << " 失败," << filePath << "打开文件失败" << endl;
				control->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
				control->http_response().set_status_code(404);
			}
		}

		void fileDownFun(google::protobuf::RpcController *control_base,
						 const HttpRequest *,
						 HttpResponse *,
						 google::protobuf::Closure *done)
		{ // 下载文件函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			if (!control->http_request().unresolved_path().empty())
			{
				string filePath = FLAGS_dataPath + control->http_request().unresolved_path();
				cout<<"filepath  is "<<filePath<<endl ;
				string suffix = filePath.substr(filePath.find_last_of('.')); // 获取文件后缀
				sendFile(control, filePath, getContentType(suffix));
			}
			else
			{
				control->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
				control->http_response().set_status_code(404);
			}
		}

		void bookDownFun(google::protobuf::RpcController *control_base,
						 const bookDownHttpRequest *request,
						 HttpResponse *,
						 google::protobuf::Closure *done)
		{ // 下载书籍函数，下载书籍单独处理
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			if (!control->http_request().unresolved_path().empty())
			{
				string filePath = FLAGS_dataPath + control->http_request().unresolved_path();
				cout<<"filepath  is "<< filePath << endl;	
				string suffix = filePath.substr(filePath.find_last_of('.')); // 获取文件后缀
				string bookId = request->bookid();
				int userId = request->userid();
				string dayTime = request->daytime();
				int count = request->count();
				BookDownloadCountTable downloadCount;
				__bookSql.get_book_by_book_id(downloadCount, bookId);
				downloadCount.dayTime = dayTime;
				++downloadCount.times;
				__bookSql.set_newest_book_count(downloadCount);
				sendFile(control, filePath, getContentType(suffix));
			}
			else
			{
				control->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
				control->http_response().set_status_code(404);
			}
		}
	};

}


