#include "slave_redis_handler.h"

using namespace std;

SlaveRedisHandler::SlaveRedisHandler(
    const std::string &ip, const int port)
    :m_client(ip, port)
{
}
SlaveRedisHandler::~SlaveRedisHandler()
{
    m_client.Close();
}

void SlaveRedisHandler::handle_read()
{
    vector<unsigned char> buf = m_client.get();
    if (buf.size() < 1)
    {
        return;
    }
    DisplayRESPData("Slave Handle Read", (char *)&buf[0], buf.size());
}

bool SlaveRedisHandler::connect()
{
    try
    {
        m_client.Connect();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR(e.what());
        return false;
    }
    return true;
}

bool SlaveRedisHandler::sendToSlaveRedis(const char* data, const size_t& data_size)
{
    m_client.put((const unsigned char *)data, data_size);
    return true;
}