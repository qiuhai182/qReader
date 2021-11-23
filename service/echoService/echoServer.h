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
#include "echo.pb.h"

using namespace std;

DEFINE_bool(echo_attachment, true, "Echo测试");
DEFINE_string(ip, "39.105.217.90", "用于文件下载的ip外网地址");
DEFINE_int32(echoPort, 8001, "服务端口");
DEFINE_string(stringPort, "8001", "服务端口");
DEFINE_string(iPort, FLAGS_ip + ":" + FLAGS_stringPort, "服务ip:port");
DEFINE_int32(idle_timeout_s, -1, "超时没有读写操作断开连接");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state ");
#define IOBuf_MAX_SIZE 253952 // IOBuf的单次读取大小

namespace echoService
{

	class EchoServiceImpl : public EchoService
	{ // 测试服务
	public:
		EchoServiceImpl(){};
		virtual ~EchoServiceImpl(){};
		virtual void EchoFun(google::protobuf::RpcController *control_base,
							 const EchoRequest *request,
							 EchoResponse *response,
							 google::protobuf::Closure *done)
		{ // 测试函数
			brpc::ClosureGuard done_guard(done);
			brpc::Controller *control = static_cast<brpc::Controller *>(control_base);
			cout << endl;
			LOG(INFO) << "\n收到请求[log_id=" << control->log_id()
					  << "] 客户端ip+port: " << control->remote_side()
					  << " 应答服务器ip+port: " << control->local_side()
					  << " (attached : " << control->request_attachment() << ")";
			LOG(INFO) << endl
					  << control->remote_side() << " 测试消息: " << request->message();
			response->set_message(request->message()); // 设置回发消息
			if (FLAGS_echo_attachment)
			{
				control->response_attachment().append(control->request_attachment());
			}
		}
	};

}
