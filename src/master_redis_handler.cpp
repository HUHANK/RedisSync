#include "master_redis_handler.h"
#include "util/mfstring.h"
#include "slave_redis_handler.h"
using namespace std;
using namespace MFShare;

MasterRedisHandler::MasterRedisHandler(const std::string &ip, const int port)
    : m_client(ip, port)
{
}

MasterRedisHandler::~MasterRedisHandler()
{
    m_client.Close();
}

bool MasterRedisHandler::connect()
{
    try
    {
        m_client.Connect();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(e.what());
        return false;
    }
    return true;
}

void MasterRedisHandler::sendPing()
{
    RedisMessage msg;
    msg << "PING";
    m_client.put(msg.pack());
}
void MasterRedisHandler::replyConfAck()
{
    RedisMessage msg;
    msg << CMD.REPLCONF << "ack" << to_string(Offset);
    m_client.put(msg.pack());
}

void MasterRedisHandler::handle_read()
{
    vector<unsigned char> buf = m_client.get();
    if (buf.size() < 1)
    {
        // 收到空的字符串不用处理
        return;
    }
    {
        string temp = "Master Handle Read(" + to_string(buf.size()) + ")";
        DisplayRESPData(temp.c_str(), (char *)&buf[0], buf.size());
    }

    const char &RESP_DATA_TYPE = buf[0];
    if (RESP_DATA_TYPE == '+')
    { // 响应单行结果
        vector<char> Func = get_line(&buf[1]);
        if (str_nicmp(&Func[0], CMD.PONG, CMD.PONG.length()))
        {
            if (!bInited)
            {
                RedisMessage msg;
                msg << CMD.REPLCONF
                    << "listening-port"
                    << to_string(m_client.GetClientPort());
                m_client.put(msg.pack());
                iInitStep++;
            }
        }
        if (str_nicmp(&Func[0], CMD.OK, CMD.OK.length()))
        {
            if (!bInited)
            {
                if (iInitStep == 1)
                {
                    RedisMessage msg;
                    msg << CMD.REPLCONF << "capa" << "psync2";
                    m_client.put(msg.pack());
                    iInitStep++;
                }
                else if (iInitStep == 2)
                {
                    RedisMessage msg;
                    msg << CMD.PSYNC << "?" << "-1";
                    m_client.put(msg.pack());
                    iInitStep++;
                }
            }
        }
        if (str_nicmp(&Func[0], CMD.FULLRESYNC, CMD.FULLRESYNC.length()))
        {
            vector<string> arr = str_split((char *)&buf[0], " ");
            if (arr.size() == 3)
            {
                // 获取Master的replid， offset
                MasterReplid = arr[1];
                Offset = str_to_uint(arr[2]);

                // 回复ACK
                replyConfAck();

                bInited = true;
                iInitStep = 0;
            }
        }
        if (str_nicmp(&Func[0], "CONTINUE", strlen("CONTINUE")))
        {
            RedisMessage msg;
            msg << "REPLCONF" << "ack" << "237150246";
            m_client.put(msg.pack());
        }
    }
    else if (RESP_DATA_TYPE == '*')
    { // 响应数组数据
        RedisMessage msg;
        msg.parse(buf);
        Offset += buf.size();
        if (pSlaveHandler)
        {
            pSlaveHandler->sendToSlaveRedis((char*)&buf[0], buf.size());
        }
        replyConfAck();
    }
}