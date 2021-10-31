#pragma once

#ifndef _BLOOMFILTER_
#define _BLOOMFILTER_

#include <math.h>
#include <vector>
using namespace std;


// 布隆过滤器负责快速检查筛选账户登录状态


// 哈希函数
unsigned int SDBMHash(const char *str);
unsigned int RSHash(const char *str);
unsigned int JSHash(const char *str);
unsigned int PJWHash(const char *str);
unsigned int APHash(const char *str);
unsigned int DJBHash(const char *str);
unsigned int ELFHash(const char *str);
unsigned int BKDRHash(const char *str);

class Bloomfilter
{
public:
    Bloomfilter(double err_rate, int num); //传入大概需要处理的字符串个数，期望的失误率，注意计算得到的哈希函数个数k需要不大于hashtable的size
    ~Bloomfilter();
    bool is_contain(const char *str); //查看字符串是否在样本中存在
    int hashnum();                    //返回哈希函数个数
    double real_precision();          //返回真实的失误率
    int sizeofpool();                 //返回bit位数

private:
    Bloomfilter() = delete;
    int addLink(char *url);                           //计算用户登陆地址，存储到bitpool
    int hashtable_init();                             //把几个哈希函数加入到vector<unsigned (*)(const char*)> hastable容器中，必须大于k
    vector<unsigned int (*)(const char *)> hashtable; //存放计算字符串哈希值的哈希函数
    int len;                                          //bit位长度
    int samplenum;                                    //样本个数，构造函数传入
    char *mypath;                                     //文件的路径，通过构造函数传入路径
    int *bitpool;                                     //在构造函数中申请的bit位
    int bitpoollen;                                   //需要的二进制位数
    int hashfuncnum;                                  //需要的哈希函数的个数 <=hashtable.size();
};
#endif
