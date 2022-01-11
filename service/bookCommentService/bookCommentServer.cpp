#include "bookCommentServer.h"
#include <ios>

int main(int argc, char *argv[])
{

  cout << endl
       << "   重庆师范大学-智能阅读器-评论服务  开始初始化" << endl
       << endl;


  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);


  brpc::Server server;
  BookCommentService::BookCommentServiceImpl comment_service_impl;     // 评论登录服务

  if (server.AddService(&comment_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) !=
      0)
  {
    LOG(ERROR) << "Fail to add commonService";
    return -1;
  }

  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;

  if (server.Start(FLAGS_commonPort, &options) != 0)
  {
    LOG(ERROR) << "Fail to start userServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
