#pragma once

#include "sight_mysql.hpp"

using namespace rapidjson;
using namespace ormpp;
using namespace std;



struct oneCsv
{
  float x;
  float y;
  string timeStamp;
  string startTime;
  string endTime;
  string bookId;
  int pageNum;
  string userId;
};


/*
 * 
 * 获得视线数据目录下的用户目录
 */
string getUserSightPath(const pageSight &sight)
{
  return (FLAGS_sightJson + sight.userId + "/");
}

/*
 * 
 * 获得对应json文件 路径+文件名
 */
string getSightJsonPath(const pageSight &sight, string &filePath)
{
  return (filePath = getUserSightPath(sight) + sight.bookId +
                     "_" + to_string(sight.pageNum) + ".json");
}

/*
 * 
 * 读取json文件内容
 */
int getSightDocumentString(const pageSight &sight, Document &document,
                           string &result)
{
  StringBuffer sbuf;
  Writer<StringBuffer> writer(sbuf);
  document.Accept(writer);
  result = sbuf.GetString();
  return 1;
}

/*
 * 
 * 读取json文件内容
 */
int getSightJsonString(const pageSight &sight, string &result)
{
  Document document;
  getSightDocumentString(sight, document, result);
  return 1;
}

/*
 * 
 * 获得一本书汇总视线数据csv文件 路径+文件名
 */
string getBookSightCsvPath(const pageSight &sight, string &filePath)
{
  return (filePath = FLAGS_sightCsv + sight.bookId + "_Sight.csv");
}

/*
 * 
 * 获得个人视线数据csv文件 路径+文件名
 */
string getUserSightCsvPath(const pageSight &sight, string &filePath)
{
  return (filePath = FLAGS_sightCsv + sight.userId + "_Sight.csv");
}

/*
 * 创建或打开json文件
 * 写入json文件
 */
int writeSightJsonDoc(const pageSight &sight, Document &document)
{
  string filePath;
  getSightJsonPath(sight, filePath);
  FILE *fp = fopen(filePath.data(), "wb");
  if (fp)
  { // 打开成功
    char writeBuffer[1024];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    Writer<FileWriteStream> writer(os);
    document.Accept(writer);
    fclose(fp);
    cout << "视线数据写入json文件: " << filePath << endl;
    return 1;
  } else
  {
    cout << "视线数据写入json文件失败: " << filePath << endl;
    return-1;
  }
}

/*
 * 
 * 确保sightJson文件存在
 */
int configureSightJson(const pageSight &sight)
{
  string jsonFile;
  getSightJsonPath(sight, jsonFile);
  configurePath(jsonFile);
  ifstream userInFile(jsonFile, ios::in | ios::binary);
  if (!userInFile.good())
  { // 文件读取失败
    Document document;
    document.Parse("{}");
    writeSightJsonDoc(sight, document); // 创建目录及json文件
    return 0;
  }
  userInFile.close();
  return 0;
}

/*
 * 
 * 确保sightCsv文件存在
 */
int configureSightCsv(const pageSight &sight)
{
  string sightFile;

  // 书籍汇总视线csv
  getBookSightCsvPath(sight, sightFile);
  configurePath(sightFile);
  ifstream bookInFile(sightFile, ios::in | ios::binary);
  if (!bookInFile.good())
  { // 文件读取失败
    ofstream bookOutFile(sightFile, ios::out | ios::binary);
    if (!bookOutFile.good())
    {
      cout << "create " << sightFile << " failed" << endl;
      bookInFile.close();
      return -1;
    }
    else
    {
      bookOutFile << "x" << ',' << "y" << ',' << "time" << ','
                  << "startTime" << ',' << "endTime" << ','
                  << "bookId" << ',' << "pageNum" << ','
                  << "userId" << endl;
      bookInFile.close();
      bookOutFile.close();
      return 0;
    }
  }
  bookInFile.close();

  // 用户汇总视线csv
  getUserSightCsvPath(sight, sightFile);
  ifstream userInFile(sightFile, ios::in | ios::binary);
  if (!userInFile.good())
  { // 文件读取失败
    ofstream userOutFile(sightFile, ios::out | ios::binary);
    if (!userOutFile.good())
    {
      cout << "create " << sightFile << " failed" << endl;
      userInFile.close();
      return -1;
    }
    else
    {
      userOutFile << "x" << ',' << "y" << ',' << "time" << ','
                  << "startTime" << ',' << "endTime" << ','
                  << "bookId" << ',' << "pageNum" << ','
                  << "userId" << endl;
      userInFile.close();
      userOutFile.close();
      return 0;
    }
  }
  userInFile.close();

  return 0;
}

/*
 * 
 * 初始化document
 */
int initDocument(const pageSight &sight, Document &document)
{
  string filePath;
  getSightJsonPath(sight, filePath);
  ifstream in(filePath.data());
  if (!in.is_open())
  {
    configureSightJson(sight);
  }
  in.open(filePath.c_str()); // 读取json数据
  if (!in.is_open())
  {
    cout << "open " << filePath << " failed." << endl;
    return -1;
  }
  istreambuf_iterator<char> begin(in);
  istreambuf_iterator<char> end;
  string json(begin, end);
  document.Parse(json.data());
  in.close();
  return 1;
}

/*
 * 写入一条csv视线数据
 * 到指定csv文件
 */
int writeSightCsv(string &csvPath, oneCsv &onecsv)
{
  ofstream outFile; // 输出csv文件文件流
  outFile.open(csvPath, ios::app | ios::binary);
  if (!outFile.good())
  {
    cout << "视线数据写入csv失败: " << csvPath << endl;
  }
  else
  {
    cout << "视线数据写入csv文件: " << csvPath << endl;
  }
  return 0;
}

/*
 * 将sight中的数据写入
 * json、csv、数据库
 */
int saveSightData(const pageSight &sight, Document *document)
{
  
  Document::AllocatorType &allocator = document->GetAllocator();
  Value bookId(kStringType);        // 书籍id
  Value pageNum;                    // 页数
  Value userId(kStringType);        // 用户id
  Value sightDataLists(kArrayType); // 视线数据列表
  Value list(kArrayType);           // 视线数据数组
  if (!(document->HasMember("bookId")))
  {
    bookId.SetString(sight.bookId.data(), allocator);
    document->AddMember("bookId", bookId,
                        allocator); // 添加书籍id属性
  }
  if (!(document->HasMember("pageNum")))
  {
    pageNum.SetInt(sight.pageNum);
    document->AddMember("pageNum", pageNum,
                        allocator); // 添加页数
  }
  if (!(document->HasMember("userId")))
  {
    userId.SetString(sight.userId.data(), allocator);
    document->AddMember("userId", userId,
                        allocator); // 添加用户id属性
  }
  if (document->HasMember("list"))
  { // 之前已有视线数据
    int size = sight.sightList.size();
    for (int i = 0; i < size; i++)
    {
      Value bufSight(kObjectType);
      bufSight.AddMember("x", (sight.sightList[i]).x, allocator);
      bufSight.AddMember("y", (sight.sightList[i]).y, allocator);
      string timeStamp = sight.sightList[i].timeStamp;
      bufSight.AddMember("timeStamp", Value(timeStamp.data(), allocator),
                         allocator);
      (*document)["list"].PushBack(bufSight, allocator); // 为document添加数据
    }
  }
  int size = sight.sightList.size();
  if (size > 0)
  {
    string startTime = sight.startTime;
    string endTime = sight.endTime;
    if (!isStampTime(startTime))
      startTime = getStampTime(atoi(startTime.data())); // timeStamp转换为日期格式
    if (!isStampTime(endTime))
      endTime = getStampTime(atoi(endTime.data())); // timeStamp转换为日期格式
    string allCsvFile;                              // 某本书汇总csv文件路径
    getBookSightCsvPath(sight, allCsvFile);
    string userCsvFile; // 个人汇总csv文件路径
    getUserSightCsvPath(sight, userCsvFile);
    for (int i = 0; i < size; i++)
    {
      // list数组中的对象，保存一个视线数据
      string timeStamp = sight.sightList[i].timeStamp; // 收到的时间，可能为日期格式，也可能为时间戳
      if (!isStampTime(timeStamp))
        timeStamp = getStampTime(atoi(timeStamp.data())); // timeStamp转换为日期格式
      Value bufSight(kObjectType);
      bufSight.AddMember("timeStamp", Value(timeStamp.data(), allocator),
                         allocator);
      bufSight.AddMember("x", (sight.sightList[i]).x, allocator);
      bufSight.AddMember("y", (sight.sightList[i]).y, allocator);
      // 添加数据到list
      list.PushBack(bufSight, allocator);
      // 属性写入csv文件
      oneCsv onecsv{sight.sightList[i].x,
                    sight.sightList[i].y,
                    timeStamp,
                    startTime,
                    endTime,
                    sight.bookId,
                    sight.pageNum,
                    sight.userId};
      writeSightCsv(allCsvFile, onecsv);
      writeSightCsv(userCsvFile, onecsv);
      // 属性写入数据库
      SightTable sightTab{sight.userId,
                          sight.bookId,
                          sight.pageNum,
                          (sight.sightList[i]).x,
                          (sight.sightList[i]).y,
                          timeStamp};
      insertSightSQL(sightTab);
    }
    document->AddMember("list", list, allocator); // 添加视线对象列表list到document
    // 该批次视线数据记录信息写入数据库
    string dayTime = endTime.substr(0, 10); // 从0开始取10位字符
    ReadSETable readSE{sight.userId, sight.bookId, sight.pageNum,
                       startTime, endTime, dayTime};
    insertSESQL(readSE);
  }
  // 写入json文件
  /*-------------暂时注释-----------------*/
  //writeSightJsonDoc(sight, *document);
  return 1;
}

/* 
 * 处理vector<tuple<string, string>>
 * 内部的所有当天的startTime、endTime
 * 并计算出12个阅读区间统计阅读时长
 * 
 */
int dealReadInterval(vector<string> &allTime, float *res)
{
  for (auto dayTime : allTime)
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
int getIntervalCount(string userId, string dayTime, float *res)
{
  auto conn = get_conn_from_pool();
  conn_guard guard(conn);
  if (conn == NULL)
  {
    cout << "FILE: " << __FILE__ << " "
         << "conn is NULL"
         << " LINE  " << __LINE__ << endl;
    return -1;
  }
  string cond = "where timeStamp LIKE \"\%" + dayTime + "\%\" and userId =  \"" + userId + "\"";
  auto result = conn->query<SightTable>(cond);
  vector<string> allTime; // 当天所有视线的时刻
  for (auto &oneres : result)
  {
    allTime.push_back(oneres.timeStamp);
  }
  return dealReadInterval(allTime, res); // 计算出统计阅读时长
}

/*
 * 客户端提交阅读数据
 * 分类写入json文件、csv文件、数据库
 */
int insertSightData(const pageSight &sight)
{ // 写入到json文件

  configureSightCsv(sight);
  configureSightJson(sight);
  Document *document;
  document = new Document();
  initDocument(sight, *document);
  saveSightData(sight, document);
  return 1;
}


/**-------------------------------------------分析部分---------------------------------------**/

//视线覆盖写入CSV文件
int writeSightTableCsv(string &csvPath,vector<SightTable>& rsc )
{
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
    outFile <<  i.userId << ',' <<  i.bookId << ','
          <<  i.pageNum << ',' <<  i.x << ','
          <<  i.y << ',' <<  i.timeStamp << endl;
  }
  return 0;
}

//存储当天Csv文件,作为处理的前提
int storageAnalyseCsv(const string &timeStamp,const string & userId)
{
  string CsvFilePath = "../../qReaderData/sightData/sightCsv/" + userId + "_Analyse.csv";
  vector<SightTable>result ;
  int ret = get_sight_by_timeStamp(result,timeStamp,userId);
  if(ret > 0){
    writeSightTableCsv(CsvFilePath,result);
    return 1;
  }else{
    return -1 ;
  }
}

//分析结果到json文件，不存在对应目录则创建usrid目录
int storageAnalyseJson(const string & userId)
{
  string filePath = "../../qReaderData/sightData/sightAnalyseJson/"+userId;
  if (eaccess(filePath.c_str(), F_OK) == -1)
  {
    int ret = mkdir(filePath.data(), 0775);
    if (ret)
    {
      cout << "FILE: " << __FILE__ << endl
            << "创建(" << filePath << ")目录错误: " << strerror(errno) << " LINE  "
            << __LINE__ << endl;
      return -1;
    }
  }

  int ret = system(string("python server.py " + userId).c_str());
	
  return ret ;
}


//获取json文本内容，储存到string
string getJsonContent(const string & filePath)
{
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



//读取展示页第二行的数据，hour,focus
void read_time_focus(map<string,string> & timeFocus,const rapidjson::Document & document,int &counter)
{
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


//读取散点图数据
void read_scatter_diagram(vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                          const rapidjson::Document & document,int &counter)
{
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
    }else{
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
    }else{
      return ;
    }   
  }
}

//读取折线图数据
void read_line_chart(vector<double> &lineChart,
                          const rapidjson::Document & document,int &counter)
{
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

//获取分析结果
int getAnalyseResult(map<string,string> & timeFocus,
                    vector<map<string,vector<map<double,double>>>> & scatterDiagram,
                    vector<double> &lineChart,
                    const string &timeStamp,const string & userId)
{
  string jsonFilePath = "../../qReaderData/sightData/sightAnalyseJson/"+userId+"/" + userId + "_Analyse.json";
  rapidjson::Document document;
  
  string json_content = getJsonContent(jsonFilePath) ;
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