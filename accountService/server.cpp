#include "server.h"
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
    // 数据库操作样例，创建并插入一条用户数据信息
    // UserInfoTable temp{
    //     "201901",
    //     "邱海",
    //     "http://121.4.52.206:8000/fileService/FileDown/default.png",
    //     "123456qh",
    //     "你好 这个人很懒~",
    //     1,
    //     1};
    // insert_user(temp); //插入一条用户信息(注册一个用户)
  }*/

  brpc::Server server;
  echoService::EchoServiceImpl echo_service_impl;          // 测试服务
  accountService::accountServiceImpl account_service_impl; // 注册登录服务
  commonService::commonServiceImpl study_service_impl;     // 注册登录服务
  bookCityService::bookCityServiceImpl
      bookcity_service_impl; // 书城(总书库)书籍管理服务
  bookShelfService::bookShelfServiceImpl
      bookshelf_service_impl;                                      // 个人书架书籍管理服务
  fileService::fileServiceImpl file_service_impl;                  // 文件传输服务
  collectdataService::collectDataServiceImpl collect_service_impl; //数据收集服务

  if (server.AddService(&echo_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) !=
      0)
  {
    LOG(ERROR) << "Fail to add EchoService";
    return -1;
  }

  if (server.AddService(&account_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add loginService";
    return -1;
  }

  if (server.AddService(&study_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) !=
      0)
  {
    LOG(ERROR) << "Fail to add studyService";
    return -1;
  }

  if (server.AddService(&bookcity_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add bookcityService";
    return -1;
  }

  if (server.AddService(&bookshelf_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add bookshelfService";
    return -1;
  }

  if (server.AddService(&file_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) !=
      0)
  {
    LOG(ERROR) << "Fail to add fileService";
    return -1;
  }
  if (server.AddService(&collect_service_impl,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
  {
    LOG(ERROR) << "Fail to add collectService";
    return -1;
  }

  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;

  if (server.Start(FLAGS_port, &options) != 0)
  {
    LOG(ERROR) << "Fail to start userServer";
    return -1;
  }

  server.RunUntilAskedToQuit();

  return 0;
}
