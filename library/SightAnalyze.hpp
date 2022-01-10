#pragma once

#ifndef _SIGHTANLYZE_H
#define _SIGHTANLYZE_H

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <typeinfo>
#include "database/sightSql.hpp"
#include "public/Times.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace ormpp;
using namespace std;
using namespace rapidjson;
using namespace Times;

DEFINE_string(sightAnalyseJsonPath,"../../../../qReaderData/sightData/sightAnalyseJson/" , "sightAnalyseJson目录");
DEFINE_string(sightCsvPath, "../../../../qReaderData/sightData/sightCsv/", "sightCsv目录");

namespace Analyze
{

    //数据库读取的字段
    struct oneSight
    {
        float x;
        float y;
        string timeStamp;
    };

    struct pageSight
    {
        int userId;
        string bookId;
        int pageNum;
        vector<oneSight> sightList;
        string startTime;
        string endTime;
    };
    
    class SightAnalyze
    {
    public:
        SightAnalyze(){};
        ~SightAnalyze(){};
        SQL_STATUS insert_sight_data(const pageSight & data);
        
        //获取分析结果
        int get_analyse_result(map<string,string> & timeFocus,
                            vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                            vector<double> &lineChart,
                            const string &timeStamp,const int & userId);
        //查询区间时间
        int get_interval_count(const int & user_id, string day_time,  float *res);

        //存储当天Csv文件,作为处理的前提
        int storage_analyse_csv(const string &timeStamp,const int & userId);
        //分析结果到json文件，不存在对应目录则创建usrid目录
        int storage_analyse_json(const int & userId);
    private:
        //视线覆盖写入CSV文件
        int write_sightTable_csv(string &csvPath,vector<SightTable>& rsc );
        //获取json文本内容，储存到string
        string get_json_content(const string & filePath);
        //读取展示页第二行的数据，hour,focus
        void read_time_focus(map<string,string> & timeFocus,const rapidjson::Document & document,int &counter);
        //读取散点图数据
        void read_scatter_diagram(vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                                const rapidjson::Document & document,int &counter);
        //读取折线图数据
        void read_line_chart(vector<double> &lineChart,
                                const rapidjson::Document & document,int &counter);
        
        
        int deal_read_interval(vector<string> &allTime, float *res);
        
    private:
        SightInfoImpl __sight;
    };




/* 
* 处理vector<tuple<string, string>>
* 内部的所有当天的startTime、endTime
* 并计算出12个阅读区间统计阅读时长
* 
*/
int SightAnalyze::deal_read_interval(vector<string> &all_time, float *res)
{
    for (auto dayTime : all_time)
    {
        char *time = dayTime.data();
        int hour = atoi(time + 11); // 小时数
        int timeId = hour / 2;
        if (timeId > 11)
        timeId = 0;
        res[timeId] += 1;
    }
    for (int i = 0; i < 12; ++i)
    {
        cout << "数量：" << res[i] << endl;
        res[i] /= 600;
    }
    cout << "统计区间阅读时长成功" << endl;
    return 1;
}


/* 
 * 查询时间区间阅读时长
 * 
 */
int SightAnalyze::get_interval_count(const int & user_id, string day_time,  float *res)
{
    vector<string>day_all_time ;

    SQL_STATUS ret = __sight.get_a_day_data(user_id, day_time,day_all_time);
    if(ret != SQL_STATUS::EXE_sus || day_all_time.size() == 0)
        return -1 ;
    return deal_read_interval(day_all_time, res); // 计算出统计阅读时长
}




SQL_STATUS SightAnalyze::insert_sight_data(const pageSight & data)
{
    
    int size = data.sightList.size();
    if (size > 0)
    {   //首先判断该段时间
        string startTime = data.startTime;
        string endTime = data.endTime;
        if (!is_stampTime(startTime))
            startTime = get_stampTime(atoi(startTime.data())); // timeStamp转换为日期格式
        if (!is_stampTime(endTime))
            endTime = get_stampTime(atoi(endTime.data())); // timeStamp转换为日期格式
        string dayTime = endTime.substr(0, 10); // 从0开始取10位字符
        ReadIntervalTable interval{data.userId, data.bookId, data.pageNum,
                                    startTime, endTime, dayTime};
        if(SQL_STATUS::EXE_sus != __sight.insert_internal_info(interval))
            return SQL_STATUS::EXE_err;
        //每条插入
        for (int i = 0; i < size; i++)
        {
            string timeStamp = data.sightList[i].timeStamp; // 收到的时间，可能为日期格式，也可能为时间戳
            if (!is_stampTime(timeStamp))
                timeStamp = get_stampTime(atoi(timeStamp.data())); // timeStamp转换为日期格式
            // 属性写入数据库
            SightTable sightTab{data.userId,data.bookId,data.pageNum,
                                (data.sightList[i]).x,(data.sightList[i]).y,timeStamp};
            __sight.insert_Sight(sightTab);
        }
    }
    return SQL_STATUS::EXE_sus;
}

int SightAnalyze::write_sightTable_csv(string &csvPath,vector<SightTable>& rsc )
{//视线覆盖写入CSV文件
    ofstream outFile; // 输出csv文件文件流
    outFile.open(csvPath, ios::ate|ios::out | ios::binary);
    if (!outFile.good())
    {
        cout << "视线数据写入csv失败: " << csvPath << endl;
    }
    else
    {
        cout << "视线数据写入csv文件: " << csvPath << endl;
    }

    cout<<rsc[1].timeStamp<<endl ;

    outFile <<"userId"<<','<<"bookId"<<','
            <<"pageNum"<<','<<"x"<<','
            <<"y"<<','<<"timeStamp"<< endl;
    for(auto i :rsc){
        outFile << to_string(i.userId) << ',' <<  i.bookId << ','
            <<  i.pageNum << ',' <<  i.x << ','
            <<  i.y << ',' <<  i.timeStamp << endl;
    }
    return 0;
}

int SightAnalyze::storage_analyse_csv(const string &timeStamp,const int & userId)
{//存储当天Csv文件,作为处理的前提
    string CsvFilePath = FLAGS_sightCsvPath + to_string(userId) + "_Analyse.csv";
    vector<SightTable>result ;
    SQL_STATUS ret = __sight.get_sight_by_timeStamp(result,timeStamp,userId);
    if(ret == SQL_STATUS::EXE_sus && result.size() > 0){
        write_sightTable_csv(CsvFilePath,result);
        return 1;
    }else{
        return -1 ;
    }
}

int SightAnalyze::storage_analyse_json(const int & userId)
{//分析结果到json文件，不存在对应目录则创建usrid目录
    string filePath = FLAGS_sightAnalyseJsonPath + to_string(userId);
    if (eaccess(filePath.c_str(), F_OK) == -1)
    {
        int ret = mkdir(filePath.data(), 0775);
        if (ret)
        {
        cout << endl
                <<"FILE: " << __FILE__ << endl
                << "创建(" << filePath << ")目录错误: " 
                << strerror(errno) << " LINE  "
                << __LINE__ << endl;
        return -1;
        }
    }

    int ret = system(string("python server.py " + userId).c_str());
        
    return ret ;
}

string SightAnalyze::get_json_content(const string & filePath)
{//获取json文本内容，储存到string
    std::ifstream in(filePath); // 文件的读取操作， copy的  不解释
    if(!in.is_open()){   
        LOG(INFO)<<"文件"<<filePath<<"打开失败"<<endl ;
    }  
    std::string json_content((std::istreambuf_iterator<char>(in)), \
                    std::istreambuf_iterator<char>());
    //将文件的数据流转换位std::string类型
    in.close();  // 关闭文件
    return json_content;//将处理之后的std::string 返回

}

void SightAnalyze::read_time_focus(map<string,string> & timeFocus,
                                const rapidjson::Document & document,int &counter)
{//读取展示页第二行的数据，hour,focus
    if ((document.HasMember("hour")))
    {
        timeFocus["hour"] = std::to_string(document["hour"].GetInt());    
        counter++;           
    }
    if ((document.HasMember("min")))
    {
        timeFocus["min"] = std::to_string(document["min"].GetInt()); 
        counter++;            
    }
    if ((document.HasMember("sec")))
    {
        timeFocus["sec"] = std::to_string(document["sec"].GetInt() );              
        counter++;
    }
    if ((document.HasMember("focus")))
    {
        timeFocus["focus"] = std::to_string(document["focus"].GetDouble());   
        counter++;      
    }
    if((document.HasMember("pages")))
    {
        timeFocus["pages"] = std::to_string(document["pages"].GetInt());
        counter++;          
    }
    if ((document.HasMember("rows")))
    {
        timeFocus["rows"] = std::to_string(document["rows"].GetInt()); 
        counter++;        
    }
}

void SightAnalyze::read_scatter_diagram(vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                                const rapidjson::Document & document,int &counter)
{//读取散点图数据
     string pre = "point";
    string actoinStr ;
    string colorStr ;
    string point ;
    vector<map<double,double> > vectorBuffer ;
    //外层类型,颜色，内层点
    for(int i = 0 ; i < 3 ; i++){
        //读取点 x,y值
        point = pre+ to_string(i);
        if ( document.HasMember(point.c_str()) )
        {
            const Value & pointArr = document[point.c_str()];
            SizeType size  =  pointArr.Size();
            for(SizeType j = 0;j < size ;j++ ){
                double x = pointArr[j][0].GetDouble() ;
                double y = pointArr[j][1].GetDouble();
                vectorBuffer.push_back(map<double,double>{{x,y}});
                counter += 2;
            }        
        }
        else
        {
            return ;
        }
        //文字字段提取
        actoinStr = pre + to_string(i)+"Action";
        colorStr = pre + to_string(i)+"Color";
        if ((document.HasMember(actoinStr.c_str())) && (document.HasMember(colorStr.c_str())) )
        {
            //便于处理将 action+color
            string conbination = document[actoinStr.c_str()].GetString() + 
                            string("+") +document[colorStr.c_str()].GetString();       
            scatterDiagram.push_back(map<string, vector<map<double,double>>>{{conbination,vectorBuffer }} );   
            counter += 2;   
            vectorBuffer.clear();//清空 
        }
        else
        {
            return ;
        }   
    }
}

void SightAnalyze::read_line_chart(vector<double> &lineChart,
                                 const rapidjson::Document & document,int &counter)
{//读取折线图数据
    if ( document.HasMember("speedPoints") )
    {
        const Value & speedPointsArr = document["speedPoints"];
        SizeType size  =  speedPointsArr.Size();
        for(SizeType i = 0; i < size ;i++ ){
        lineChart.push_back(speedPointsArr[i][0].GetDouble());
        counter++;
        }        
    }else{
        return ;
    }
}

int SightAnalyze::get_analyse_result(map<string,string> & timeFocus,
                                vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                                vector<double> &lineChart,
                                const string &timeStamp,const int & userId)  
{//获取分析结果
    string jsonFilePath = FLAGS_sightAnalyseJsonPath + to_string(userId)+"/" + to_string(userId) + "_Analyse.json";
    rapidjson::Document document;
    
    string json_content = get_json_content(jsonFilePath) ;
    document.Parse(json_content.c_str());

    int counter = 0 ;
    //专注度
    read_time_focus(timeFocus,document,counter);
    //散点
    read_scatter_diagram(scatterDiagram,document,counter);
    //折线图
    read_line_chart(lineChart,document,counter);
    
    return counter;
}  



}




#endif
