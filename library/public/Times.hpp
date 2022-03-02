
#pragma once

#ifndef _TIMES_H
#define _TIMES_H

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <list>
#include <array>
#include <set>
#include <functional>
#include <thread>
#include <mutex>
#include <exception>
#include <stdexcept>
#include <chrono>


const int maxLength = 1024; // 某些数据的最大长度

using namespace std;

namespace Times
{

    // template <typename T>
    // const LockSet
    // {
    // public:
    //     LockSet(){};
    //     ~LockSet(){};
    //     int size()
    //     {

    //     }
    
    // private:
    //     std::set<T> __set;
    // }

    struct TimePos
    {
        //毫秒，秒，分钟
        int ms;
        int s;
        int min;

    };

    struct EventData
    {
        //超时时间(为初始化的倍数)
        int interval;
        //回调函数
        std::function<void(void*)>  cb_func;
        //时间
        TimePos timePos;
        //定时器ID
        int timerId;
        //参数 
        void * arg;

    };

    class TimeWheel
    {
    public:
        TimeWheel() = delete;
        /*step为心跳检测的步长，max_min为分钟时间轮槽数*/
        TimeWheel(const int &step, const int& max_min);
        ~TimeWheel();
    public:
        int AddTimer( EventData  event);
        int DeleteTimer(const int &timerId);
    private:
        int Tick();
        int GenerateTimerID();
        void InsertTimer(const int &diff_ms,EventData &event);
        void GetNextTrigerPos(const int &interval,TimePos &timePos);
        int GetMS(const TimePos & timePos);
        void DealEvent( std::list<EventData> events);
    private:
        //为对应的槽
        std::list<EventData> *__time_slots_header{nullptr};
        std::mutex			  __mutex;
        //维护的内部时间，不同步获取
        TimePos               __time_pos;
        
        //对应的槽数
        int __ms_slots{0};
        int __s_slots{0};
        int __min_slots{0};
        //步长
        int __step{0};
        //时间轮中定时器数量
        int __timer_count{0};
    };

   
    /**************************************************************************/
    /*******************************public*************************************/
    /**************************************************************************/

    TimeWheel::TimeWheel(const int & step,const  int& max_min)
    {
        memset(&__time_pos, 0, sizeof(__time_pos));
        if (1000 % step != 0)
        {//异常 终止构造
                throw  out_of_range("The parameter step is not divisible");
        }

        //步长
        __ms_slots = 1000 / step;;
        __s_slots = 60;
        __min_slots = max_min;
        //事件
        //初始化链表的数组，数组里的元素为链表,时间轮数组
        __time_slots_header = new std::list<EventData>[__ms_slots + __s_slots + __min_slots];
        __step = step;
        std::thread th([&]{
            this->Tick();
        });

        th.detach(); 
    }


    TimeWheel::~TimeWheel()
    {
    }

    int TimeWheel::AddTimer(EventData  event)
    {//向容器添加定时，超时时间必须为步长的整数倍
    //函数成功返回生成的定时器ID
        if (event.interval < __step 
        || event.interval % __step != 0 
        || event.interval >= __step * __ms_slots * __s_slots * __min_slots)
        {
            return -1;
        }

        std::unique_lock<std::mutex> lock(__mutex);
        event.timePos = __time_pos ;

        event.timerId = GenerateTimerID();

        
        InsertTimer(event.interval, event);
        __timer_count++;

        return event.timerId;

    }

    
    int TimeWheel::DeleteTimer(const int &timerId)
    {//将容器从定时器中删除
        std::unique_lock<std::mutex> lock(__mutex);
        int index = 0;
        //最大事件个数
        int slots_count = __ms_slots + __s_slots + __min_slots;
        for (index = 0; index < slots_count; index++)
        {
            std::list<EventData>& events = __time_slots_header[index];
            for (auto item = events.begin(); item != events.end(); item++)
            {
                if (item->timerId == timerId)
                {
                    item = events.erase(item);
                    return 0;
                }
            }
        }
        if (index == slots_count)
        {
            //std::cout << "timer not found" << std::endl;
            return -1;
        }
        return 0;
    }

    /**************************************************************************/
    /*******************************private************************************/
    /**************************************************************************/


    int TimeWheel::Tick()
    {//步长心搏检测
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(__step));
            std::unique_lock<std::mutex> lock(__mutex);
            TimePos pos;
            memset(&pos, 0, sizeof(pos));

            TimePos last_pos = __time_pos;
            GetNextTrigerPos(__step, pos);
            __time_pos = pos;

            if (pos.min != last_pos.min)
            {
                std::list<EventData>& events = __time_slots_header[__time_pos.min + __s_slots + __ms_slots];
                DealEvent(events);
                __timer_count -= events.size();
                events.clear();
            }
            else if (pos.s != last_pos.s)
            {
                std::list<EventData>& events = __time_slots_header[__time_pos.s + __ms_slots];
                DealEvent(events);
                __timer_count -= events.size();
                events.clear();
            }
            else if (pos.ms != last_pos.ms)
            {
                std::list<EventData>& events = __time_slots_header[__time_pos.ms];
                DealEvent(events);
                __timer_count -= events.size();
                events.clear();
            }
            else
            {
                return -1;
            }
            //cout<<"tick  count "<<__timer_count<< endl;
            lock.unlock();
        }
        return 0;
    }

    
    int TimeWheel::GenerateTimerID()
    {//生成ID
        srand((int)time(0));
        int x = rand() % 0xffffffff;
        int cur_time = static_cast<int>(time(nullptr));
        return x | cur_time | __timer_count;
    }

    
    void TimeWheel::InsertTimer(const int &diff_ms, EventData &event)
    {//插入定时器
        TimePos timePos = { 0 };

        GetNextTrigerPos(diff_ms, timePos);

        //数组的每一个元素都是一个list链表
        if (timePos.min != __time_pos.min)
            __time_slots_header[__ms_slots + __s_slots + timePos.min].push_back(event);
        else if (timePos.s != __time_pos.s)
            __time_slots_header[__ms_slots + timePos.s].push_back(event);
        else if (timePos.ms != __time_pos.ms)
            __time_slots_header[timePos.ms].push_back(event);

    }


    
    void TimeWheel::GetNextTrigerPos(const int & interval, TimePos &timePos)
    {//获得下一个触发超时时间。
        
        int cur_ms = GetMS(__time_pos);
        int future_ms = cur_ms + interval;

        timePos.min = (future_ms / 1000 / 60) % __min_slots;
        timePos.s = (future_ms % (1000 * 60)) / 1000;
        timePos.ms = (future_ms % 1000) / __step;
    }


    
    int TimeWheel::GetMS(const TimePos & timePos)
    {//将时刻转为毫秒
        return __step * timePos.ms + timePos.s * 1000 + timePos.min * 60 * 1000;
    }


    
    void TimeWheel::DealEvent(std::list<EventData>  events)
    {//超时事件
        for (auto item = events.begin(); item != events.end(); item++)
        {
            int cur_ms = GetMS(__time_pos);
            int last_ms = GetMS(item->timePos);
            int diff_ms = (cur_ms - last_ms + (__min_slots + 1) * 60 * 1000) % ((__min_slots + 1) * 60 * 1000);
            if (diff_ms == item->interval)
            {
                item->cb_func(item->arg);
                item->timePos = __time_pos;
                //InsertTimer(item->interval, *item);
            }
            else
            {
                InsertTimer(item->interval - diff_ms, *item);
            }
            //cout<<" size "<<__timer_count<<endl;
        }

    }




}


namespace Times
{
    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(char *timeString)
    {
        int iY = atoi(timeString); // atoi函数遇到非数字自动结束取位
        int iM = atoi(timeString + 5);
        int iD = atoi(timeString + 8);
        int iH = atoi(timeString + 11);
        int iMin = atoi(timeString + 14);
        int iS = atoi(timeString + 17);
        struct tm timeInfo;
        memset(&timeInfo, 0, sizeof(timeInfo));
        timeInfo.tm_year = iY - 1900;
        timeInfo.tm_mon = iM - 1;
        timeInfo.tm_mday = iD;
        timeInfo.tm_hour = iH;
        timeInfo.tm_min = iMin;
        timeInfo.tm_sec = iS;
        return mktime(&timeInfo);
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(const char *timeString)
    {
        return get_timeStamp(const_cast<char *>(timeString));
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(const string &timeString)
    {
        char timeChar[maxLength];
        for (int i = 0; i < timeString.size(); ++i)
        {
            timeChar[i] = timeString.data()[i];
        }
        return get_timeStamp(timeChar);
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(string &timeString)
    {
        return get_timeStamp(timeString.data());
    }

    /*
    * 判断收到的时间是否是日期格式
    * 是则返回True
    * 若是时间戳则返回False
    */
    bool is_stampTime(string recvTime)
    {
        string id = recvTime.substr(4, 1);
        return "-" == id;
    }

    /*
    * 根据int时间戳
    * 得到string时间
    */
    string get_stampTime(int timeStamp)
    {
        char res[1024];
        time_t tick = (time_t)timeStamp;
        struct tm timeInfo = *localtime(&tick);
        strftime(res, sizeof(res), "%Y-%m-%d %H:%M:%S", (struct tm *)&timeInfo);
        return res;
    }
    
    /*
     * 得到此刻时间：xxxx-xx-xx
     * 
     */
    string get_nowTime()
    {
        time_t timeValue = 0;
        time(&timeValue);
        struct tm *curTime = gmtime(&timeValue);
        int year = curTime->tm_year + 1900;
        int month = curTime->tm_mon + 1;
        int day = curTime->tm_mday;
        int weekenday = curTime->tm_wday;
        int hour = curTime->tm_hour;
        int minute = curTime->tm_min;
        int second = curTime->tm_sec;
        string preMonth = month > 9 ? "" : "0";
        string preDay = day > 9 ? "" : "0";
        return string("" + to_string(year) + "-" + preMonth + to_string(month) + "-" + preDay + to_string(day));
    }

    /*
     * 得到此刻时间戳
     * 
     */
    int get_nowTimeStamp()
    {
        time_t timeValue = 0;
        time(&timeValue);
        struct tm *curTime = gmtime(&timeValue);
        int year = curTime->tm_year + 1900;
        int month = curTime->tm_mon + 1;
        int day = curTime->tm_mday;
        int weekenday = curTime->tm_wday;
        int hour = curTime->tm_hour;
        int minute = curTime->tm_min;
        int second = curTime->tm_sec;
        string preMonth = month > 9 ? "" : "0";
        string preDay = day > 9 ? "" : "0";
        string preHour = hour > 9 ? "" : "0";
        string preMinute = minute > 9 ? "" : "0";
        string preSecond = second > 9 ? "" : "0";
        string timeString = to_string(year) + "-" + preMonth + to_string(month) + "-" + preDay + 
                            to_string(day) + " " + preHour + to_string(hour) + "-" + preMinute + 
                            to_string(minute) + "-" + preSecond + to_string(second);
        return get_timeStamp(timeString);
    }
}
     
#endif

