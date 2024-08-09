#pragma once
#include <cstring>
#include <vector>
#include <string>

class CCMD
{
public:
    const std::string OK = "OK";
    const std::string PING = "PING";
    const std::string PONG = "PONG";
    const std::string REPLCONF = "REPLCONF";
    const std::string PSYNC = "PSYNC";
    const std::string FULLRESYNC = "FULLRESYNC";
    const std::string SELECT = "SELECT";
    const std::string CONTINUE = "CONTINUE";
};

extern CCMD CMD;

// 目前Redis的通信协议采用的是RESP协议
class RedisMessage
{
public:
    RedisMessage(const int& type = 0);
    ~RedisMessage();

    RedisMessage& AddFunc(const std::string& funcName);
    RedisMessage& AddParam(const std::string& param);
    RedisMessage& AddParam(const char* param, const int len);

    RedisMessage& operator<<(const std::string& data);

    std::vector<char> pack();

    void clear();

    bool parse(const std::vector<unsigned char>& buf);

    std::vector<char> getParam(const int& index);
    int getParamSize();
private:
    // string转vector<char>
    std::vector<char> StrToVec(const std::string& str);
private:
    // 用于发送的参数
    std::vector<std::vector<char>> mParams;
    // 用于发送的参数的长度
    std::vector<int> mParamLens;

    const int Type = 0; // 0: 请求包 1: 回复包
};

void DisplayRESPData(const char* ExtMsg, const char* buf, const int& len);