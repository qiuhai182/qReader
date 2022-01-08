#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "database/tableInfo/UserInfo.hpp"
#include "database/tableInfo/BookBaseInfo.hpp"
#include "database/tableInfo/BookSearchInfo.hpp"
#include "database/tableInfo/BookGradeInfo.hpp"
#include "database/tableInfo/BookShelfInfo.hpp"
#include "database/tableInfo/SightInfo.hpp"
#include "database/tableInfo/ReadIntervalInfo.hpp"
#include "database/tableInfo/BookCommentHitInfo.hpp"
#include "database/tableInfo/BookCommentHitCountInfo.hpp"


using namespace std;
using namespace ormpp;

namespace Init 
{
   

  void createSql()
  {
    try{
        BookBaseInfo baseBook(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }
    
    try{
        UserInfo userInfo(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        UserShelfInfo userShelfInfo(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        BookGradeInfo gradeBook(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }
    
    try{
        BookCommentHitInfo gradeBook(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        BookCommentHitCountInfo hitCount(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        BookSearchInfo searchInfo(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        SightInfo searchInfo(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

    try{
        ReadIntervalInfo intervalInfo(false);
        usleep(1000);
    }catch(const char *error){
        cout<<"Error: "<<error<<endl;
    }

  }
}

