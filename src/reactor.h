#pragma once
#include <vector>
#include <map>
#include <string>
#include "net.h"

class EventHandler
{
public:
    virtual std::string Name() {}

    virtual void handle_read() {}
    virtual void handle_write() {}
    virtual void handle_error() {}

    virtual CNetTcpClientWithBuffer& get_client(){}

protected:
    std::vector<char> get_line(const void* buf);
};

class Reactor
{
public:
    Reactor();
    ~Reactor();

    bool register_handler(EventHandler *);
    void remove_handler(EventHandler *);

    void run_forever();
    void stop();

private:
    void dispatch();

private:
    std::vector<EventHandler*> m_handlers;
    bool m_bRun = true;

};