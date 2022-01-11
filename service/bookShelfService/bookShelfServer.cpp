#include "bookShelfServer.h"
#include <ios>

int main(int argc, char *argv[])
{

  cout << endl
       << "   重庆师范大学-智能阅读器-书架服务  开始初始化" << endl
       << endl;


  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  brpc::Server server;

  bookShelfService::bookShelfServiceImpl
      bookshelf_service_impl;                                      // 个人书架书籍管理服务



  if (server.AddService(&bookshelf_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add bookshelfService";
    return -1;
  }


  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;

  if (server.Start(FLAGS_bookShelfPort, &options) != 0)
  {
    LOG(ERROR) << "Fail to start userServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
