#include "redis_message.h"
#include <stdint.h>
#include <stdarg.h>
#include <iostream>
#include "util/mfstring.h"
#include "log/mflog.h"

using namespace std;
using namespace MFShare;

CCMD CMD;

RedisMessage::RedisMessage(const int &type)
    : Type(type)
{
}

RedisMessage::~RedisMessage()
{
    this->clear();
}

std::vector<char> RedisMessage::StrToVec(const std::string &str)
{
    return vector<char>(str.begin(), str.end());
}

RedisMessage &RedisMessage::AddFunc(const std::string &funcName)
{
    auto param = StrToVec(funcName);
    mParamLens.push_back(param.size());
    mParams.push_back(param);
    return *this;
}

RedisMessage &RedisMessage::AddParam(const std::string &param)
{
    return AddFunc(param);
}

RedisMessage &RedisMessage::AddParam(const char *param, const int len)
{
    mParamLens.push_back(len);
    mParams.push_back(vector<char>(param, param + len));
    return *this;
}

RedisMessage &RedisMessage::operator<<(const std::string &data)
{
    this->AddParam(data);
    return *this;
}

void RedisMessage::clear()
{
    mParams.clear();
    mParamLens.clear();
}

bool RedisMessage::parse(const std::vector<unsigned char> &buf)
{
    if (buf.size() < 1)
    {
        return false;
    }

    int ArrSize = -1;
    int ArrItemSize = -1;
    string temp = "";

    for (int i = 0; i < buf.size(); i++)
    {
        if (i + 1 < buf.size())
        {
            if (buf[i] == '\r' && buf[i + 1] == '\n')
            {
                if (ArrSize < 0)
                {
                    ArrSize = str_to_int(temp);
                }
                else if (ArrItemSize < 0)
                {
                    ArrItemSize = str_to_int(temp);
                    mParamLens.push_back(ArrItemSize);
                }
                else if (ArrItemSize >= 0)
                {
                    if (ArrItemSize != temp.length())
                    {
                        LOG_ERROR("Parse Redis message error: %s", temp.c_str());
                    }
                    mParams.push_back(StrToVec(temp));
                    if (mParams.size() == ArrSize)
                    {
                        // 退出
                        break;
                    }
                }
                temp = "";
                i++;
                continue;
            }

            if (ArrSize < 0)
            {
                if (buf[i] == '*')
                {
                    continue;
                }
                temp.push_back(buf[i]);
            }
            else
            {
                if (buf[i] == '$')
                {
                    ArrItemSize = -1;
                    continue;
                }
                temp.push_back(buf[i]);
            }
        }
    }
    return true;
}

std::vector<char> RedisMessage::pack()
{
    const int CNT = mParams.size();
    const vector<char> SEP = {'\r', '\n'};
    vector<char> buf;

    if (Type == 0)
    {
        vector<char> temp = StrToVec(str_combine("*%i\r\n", CNT));
        buf.insert(buf.end(), temp.begin(), temp.end());

        for (int i = 0; i < CNT; i++)
        {
            temp = StrToVec(str_combine("$%d\r\n", mParamLens[i]));
            buf.insert(buf.end(), temp.begin(), temp.end());
            buf.insert(buf.end(), mParams[i].begin(), mParams[i].end());
            buf.insert(buf.end(), SEP.begin(), SEP.end());
        }
    }
    else if (Type == 1)
    {
        if (CNT == 1)
        {
            buf.insert(buf.end(), mParams[0].begin(), mParams[0].end());
            buf.insert(buf.end(), SEP.begin(), SEP.end());
        }
    }

    DisplayRESPData("Send Message", &buf[0], buf.size());
    return buf;
}

std::vector<char> RedisMessage::getParam(const int &index)
{
    if (index >= 0 && index < mParams.size())
    {
        return mParams[index];
    }
    return vector<char>();
}
int RedisMessage::getParamSize()
{
    return mParams.size();
}

void DisplayRESPData(const char* ExtMsg, const char* buf, const int& len)
{
    string str;
    str.resize(len+1);
    memcpy(&str[0], buf, len);
    str[len] = '\0';
    str = str_replace_all(str, "\r", "\\r");
    str = str_replace_all(str, "\n", "\\n");
    LOG_DEBUG("%s RESP: <%s>", ExtMsg, str.c_str());
}