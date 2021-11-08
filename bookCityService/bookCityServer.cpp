/*
 * @Author:zqj
 * @Date: 2021-11-06 00:15:28
 * @LastEditTime: 2021-11-07 00:12:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /oxc/qReader_o/bookCityService/bookcity.cpp
 */
#include "bookCityServer.h"
#include <ios>

int main(int argc, char *argv[])
{

  cout << endl
       << "   重庆师范大学-智能阅读器-书城服务  开始初始化" << endl
       << endl;

  initDataPath(); // 初始化函数

  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  /*cout << "   创建数据库表格" << endl;
  if (1 == create_tables()) { // 创建所有需要的表格
  }*/

  brpc::Server server;
  bookCityService::bookCityServiceImpl
      bookcity_service_impl; // 书城(总书库)书籍管理服务

  if (server.AddService(&bookcity_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add bookcityService";
    return -1;
  }


  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;

  if (server.Start(FLAGS_bookCityport, &options) != 0)
  {
    LOG(ERROR) << "Fail to start userServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
