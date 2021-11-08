/*
 * @Author: your name
 * @Date: 2021-11-06 02:04:58
 * @LastEditTime: 2021-11-06 02:13:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /qReader/accountService/server.cpp
 */
#include "accountServer.h"
#include <ios>

int main(int argc, char *argv[])
{

  cout << endl
       << "   重庆师范大学-智能阅读器-服务器  开始初始化" << endl
       << endl;

  initDataPath(); // 初始化函数

  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  /*cout << "   创建数据库表格" << endl;
  if (1 == create_tables()) { // 创建所有需要的表格
    LOG(INFO) << "输出信息：已创建所有数据库表格";
  }*/

  brpc::Server server;
  accountService::accountServiceImpl account_service_impl; // 注册登录服务

  if (server.AddService(&account_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add loginService";
    return -1;
  }

  

  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;

  if (server.Start(FLAGS_accountPort, &options) != 0)
  {
    LOG(ERROR) << "Fail to start userServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
