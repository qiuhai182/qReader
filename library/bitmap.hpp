#pragma once

#include <iostream>
#include <vector>
using namespace std;
#define size_bitmap 4294967296


// bitmap位运算负责快速检查筛选账户登录状态


class BitMap
{
public:

    BitMap()
    {
        _v.resize((size_bitmap >> 5) + 1); // 相当于size_bitmap/32 + 1
    }

    void Set(size_t ip, size_t port) // 设置值为1
    {
        Set(ip);
        Set(port);
    }

    void Set(size_t num)
    {
        size_t index = num >> 5; // 相当于num/32
        size_t pos = num % 32;
        _v[index] |= (1 << pos);
    }

    void ReSet(size_t num) //重置，设置值为0
    {
        size_t index = num >> 5; // 相当于num/32
        size_t pos = num % 32;
        _v[index] &= ~(1 << pos);
    }

    bool HasExisted(size_t num) //搜索检查是否存在值
    {
        size_t index = num >> 5;
        size_t pos = num % 32;
        bool flag = false;
        if (_v[index] & (1 << pos))
            flag = true;            
        return flag;
    }

private:
    vector<size_t> _v;

};
