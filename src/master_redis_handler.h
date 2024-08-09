#pragma once
#include <string>
#include "reactor.h"
#include "net.h"
#include "redis_message.h"


class SlaveRedisHandler;
class MasterRedisHandler: public EventHandler
{
public:
    MasterRedisHandler(const std::string& ip, const int port);
    ~MasterRedisHandler();

    std::string Name() { return "MasterRedisHandler"; }

    void setSlaveHandler(SlaveRedisHandler* p) { pSlaveHandler = p; }

    bool connect();

    void sendPing();
    void replyConfAck();

    CNetTcpClientWithBuffer& get_client() override { return m_client; }

    void handle_read() override;
private:
    CNetTcpClientWithBuffer m_client;
    bool bInited = false;
    int iInitStep = 0;
    std::string MasterReplid = "";
    size_t Offset;
    SlaveRedisHandler* pSlaveHandler = nullptr;
};