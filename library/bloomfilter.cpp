#include "bloomfilter.h"
#include <iostream>
#include <string>
#include <math.h>
#include <stdio.h>
#include <fstream>
using namespace std;


// 布隆过滤器负责快速检查筛选账户登录状态


double lg2(double n)
{
    return log(n) / log(2);
}

int Bloomfilter::hashtable_init()
{
    hashtable.push_back(*PJWHash);
    hashtable.push_back(*JSHash);
    hashtable.push_back(*RSHash);
    hashtable.push_back(&SDBMHash);
    hashtable.push_back(*APHash);
    hashtable.push_back(*DJBHash);
    hashtable.push_back(*BKDRHash);
    hashtable.push_back(*ELFHash);
    return hashtable.size();
}

// 期望错误率、大致需要处理的字符串数量
Bloomfilter::Bloomfilter(double err_rate, int num)
    : samplenum(num)
{
    bitpoollen = -((samplenum * log(err_rate)) / (log(2) * log(2))); // 计算需要的二进制位数
    hashfuncnum = 0.7 * (bitpoollen / samplenum);                    // 计算需要的哈希函数个数
    len = bitpoollen / 32 + 1;                                       // 计算需要的bit位长度
    bitpool = new int[len];                                          // 申请bit位
    hashtable_init();                                                // 初始化
    if (hashfuncnum > hashtable.size())
    {
        cout << "期望的错误率过低 或 哈希表中的哈希函数不足" << endl;
        exit(0);
    }
}

int Bloomfilter::addLink(char *url)
{ // 添加链接
    for (int i = 0; i != hashfuncnum; i++)
    {
        int hashval = hashtable[i](url);
        hashval = hashval % (len * 32);
        bitpool[hashval / 32] |= (0x1 << (hashval % 32));
    }
}

int Bloomfilter::hashnum()
{ // 返回哈希函数个数
    return hashfuncnum;
}

int Bloomfilter::sizeofpool()
{ // bit位长度
    return len;
}

bool Bloomfilter::is_contain(const char *str)
{ // 判断是否存在该值
    for (int i = 0; i != hashfuncnum; i++)
    {
        int hashval = hashtable[i](str);
        hashval = hashval % (len * 32); // len*32为bitpool的总位数
        if (bitpool[hashval / 32] & (0x1 << (hashval % 32)))
            continue;
        else
            return false;
    }
    return true;
}

Bloomfilter::~Bloomfilter()
{
    delete[] bitpool;
}

unsigned int SDBMHash(const char *str)
{
    unsigned int hash = 0;
    while (*str)
    {
        // equivalent to: hash = 65599*hash + (*str++);
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    }
    return (hash & 0x7FFFFFFF);
}

// RS Hash Function
unsigned int RSHash(const char *str)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * a + (*str++);
        a *= b;
    }
    return (hash & 0x7FFFFFFF);
}

// JS Hash Function
unsigned int JSHash(const char *str)
{
    unsigned int hash = 1315423911;
    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }
    return (hash & 0x7FFFFFFF);
}

// P. J. Weinberger Hash Function
unsigned int PJWHash(const char *str)
{
    unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    unsigned int ThreeQuarters = (unsigned int)((BitsInUnignedInt * 3) / 4);
    unsigned int OneEighth = (unsigned int)(BitsInUnignedInt / 8);
    unsigned int HighBits = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
    unsigned int hash = 0;
    unsigned int test = 0;
    while (*str)
    {
        hash = (hash << OneEighth) + (*str++);
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }
    return (hash & 0x7FFFFFFF);
}

// ELF Hash Function
unsigned int ELFHash(const char *str)
{
    unsigned int hash = 0;
    unsigned int x = 0;
    while (*str)
    {
        hash = (hash << 4) + (*str++);
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }
    return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
unsigned int BKDRHash(const char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}

// DJB Hash Function
unsigned int DJBHash(const char *str)
{
    unsigned int hash = 5381;
    while (*str)
    {
        hash += (hash << 5) + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}

// AP Hash Function
unsigned int APHash(const char *str)
{
    unsigned int hash = 0;
    int i;
    for (i = 0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }
    return (hash & 0x7FFFFFFF);
}
