#pragma once
#include <string>
#include "reactor.h"
#include "net.h"
#include "redis_message.h"

class SlaveRedisHandler: public EventHandler
{
public:
    SlaveRedisHandler(const std::string &ip, const int port);
    ~SlaveRedisHandler();

    std::string Name() { return "SlaveRedisHandler"; }

    void handle_read() override;

    bool connect();
    bool sendToSlaveRedis(const char* data, const size_t& data_size);
    CNetTcpClientWithBuffer& get_client() override { return m_client; }
private:
    CNetTcpClientWithBuffer m_client;
};