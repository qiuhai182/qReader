/*
 * @Author: zqj
 * @Date: 2021-11-06 00:22:55
 * @LastEditTime: 2021-11-07 00:17:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /qReader_o/bookShelfService/bookShelfServer.cpp
 */
#include "bookShelfServer.h"
#include <ios>

int main(int argc, char *argv[])
{

  cout << endl
       << "   重庆师范大学-智能阅读器-书架服务  开始初始化" << endl
       << endl;

  initDataPath(); // 初始化函数

  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  /*cout << "   创建数据库表格" << endl;
  if (1 == create_tables()) { // 创建所有需要的表格
    LOG(INFO) << "输出信息：已创建所有数据库表格";
  }*/

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
