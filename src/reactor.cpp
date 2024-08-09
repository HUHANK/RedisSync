#include "reactor.h"
#include <thread>
using namespace std;


std::vector<char> EventHandler::get_line(const void* buf)
{
    std::vector<char> ret;
    if (buf == nullptr)
    {
        return ret;
    }

    char* pc = (char*)buf;
    while(!(pc[0] == '\r' && pc[1] == '\n'))
    {
        ret.push_back(pc[0]);
        pc++;
    }
    ret.push_back('\0');
    return ret;
}



Reactor::Reactor()
{
}

Reactor::~Reactor()
{
    this->stop();
}

bool Reactor::register_handler(EventHandler * handler)
{
    if (!handler)
    {
        return false;
    }

    for (auto ph : m_handlers)
    {
        if (ph == handler)
        {
            return false;
        }
    }

    m_handlers.push_back(handler);
    return true;
}
void Reactor::remove_handler(EventHandler * handler)
{
    vector<EventHandler *>::iterator it;
    for(it = m_handlers.begin(); it != m_handlers.end(); it++)
    {
        if (*it == handler)
        {
            break;
        }
    }
    if (it != m_handlers.end())
    {
        m_handlers.erase(it);
    }
}

void Reactor::run_forever()
{
    while(m_bRun)
    {
        this->dispatch();
        this_thread::sleep_for(1us);
    }
}
void Reactor::stop()
{
    m_bRun = false;
    for(auto pHandler : m_handlers)
    {
        if(!pHandler)
        {
            continue;
        }
    }
    m_handlers.clear();
}

void Reactor::dispatch()
{
    for(auto pHandler : m_handlers)
    {
        if (!pHandler)
        {
            continue;
        }
        
        // cout << "Dispatch " << pHandler->Name() << endl;
        pHandler->get_client().RecvFromSocket();
        if (pHandler->get_client().CanRead())
        {
            pHandler->handle_read();
        }
        if (pHandler->get_client().CanWrite())
        {
            pHandler->get_client().SendToSocket();
        }
    }
}