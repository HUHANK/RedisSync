#include "net.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
using namespace std;

#define LAST_ERROR strerror(errno)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

CNetTcp::CNetTcp()
{
    signal(SIGPIPE, SIG_IGN);
}

CNetTcp::~CNetTcp()
{
    this->Close();
}

void CNetTcp::Close()
{
    if (FD > 0)
    {
        //::shutdown(FD, 1);
        ::close(FD);
        FD = -1;
    }
}

string CNetTcp::to_string() const
{
    return HOST + ":" + ::to_string(PORT);
}

bool CNetTcp::SetNonBlock()
{
    if (FD < 0)
    {
        LOG_ERROR("SetNonBlock failed, FD = %d", FD);
        return false;
    }

    int flags = fcntl(FD, F_GETFL, 0);
    if (flags < 0)
    {
        LOG_ERROR("Get Socket flags failed, FD = %d, error = %s",
                  FD, strerror(errno));
        flags = 0;
    }

    flags |= O_NONBLOCK;
    int ret = fcntl(FD, F_SETFL, flags);
    if (ret < 0)
    {
        LOG_ERROR("Set Socket flags failed, FD = %d, error = %s",
                  FD, strerror(errno));
        return false;
    }
    return true;
}

bool CNetTcp::SetAddrReuse()
{
    if (FD < 0)
    {
        LOG_ERROR("SetAddrReuse failed, FD = %d", FD);
        return false;
    }

    int optval = 1;
    int ret = setsockopt(FD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0)
    {
        LOG_ERROR("Set Socket reuse option failed, FD = %d, error = %s",
                  FD, strerror(errno));
        return false;
    }
    return true;
}

bool CNetTcp::SetRecvBufSize(const unsigned long size)
{
    socklen_t optlen = sizeof(size);
    int err = setsockopt(FD, SOL_SOCKET, SO_RCVBUF, &size, optlen);
    if (err)
    {
        LOG_ERROR("Set socket recv buffer size failed, error = %s", strerror(errno));
        return false;
    }
    return true;
}
bool CNetTcp::SetSendBufSize(const unsigned long size)
{
    socklen_t optlen = sizeof(size);
    int err = setsockopt(FD, SOL_SOCKET, SO_SNDBUF, &size, optlen);
    if (err)
    {
        LOG_ERROR("Set socket send buffer size failed, error = %s", strerror(errno));
        return false;
    }
    return true;
}

/*********************************TCP Server**************************************/
CNetTcpServer::CNetTcpServer(const int port)
    : CNetTcp()
{
    PORT = port;
    TYPE = NetType::TCP_Server;
}

CNetTcpServer::~CNetTcpServer()
{
}

bool CNetTcpServer::InitServer()
{
    if (FD > 0)
    {
        return true;
    }

    FD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (FD < 0)
    {
        LOG_ERROR("Get Socket failed, error = %s", strerror(errno));
        return false;
    }

    if (!this->SetNonBlock())
    {
        Close();
        return false;
    }

    if (!this->SetAddrReuse())
    {
        Close();
        return false;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = ::bind(FD, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERROR("Bind port[%d] failed, error = %s", PORT, strerror(errno));
        Close();
        return false;
    }

    ret = ::listen(FD, 1);
    if (ret != 0)
    {
        LOG_ERROR("Listen port[%d] failed, error = %s", PORT, strerror(errno));
        Close();
        return false;
    }

    return true;
}

shared_ptr<CNetTcpClientWithBuffer> CNetTcpServer::Accept()
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t len = sizeof(addr);

    int fd = ::accept(FD, (struct sockaddr *)&addr, &len);
    if (fd < 0)
    {
        if (errno == EWOULDBLOCK || errno == EPROTO || errno == EINTR || errno == EAGAIN)
        {
            // EWOULDBLOCK (Berkeley实现，客户端中止连接时)、 ECONNABORTED (POSIX实现，客户中止连接时)
            // EPROTO(SVR4实现，客户端中止连接时) 和 EINTR(如果信号被捕获)
            return nullptr;
        }
        LOG_ERROR("socket accept failed, error = %s(%d)", LAST_ERROR, errno);
        return nullptr;
    }

    LOG_INFO("Accept new request, %s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return make_shared<CNetTcpClientWithBuffer>(fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

/*********************************CNetTcpClientWithBuffer**************************************/
CNetTcpClientWithBuffer::CNetTcpClientWithBuffer(const std::string &host, const int port)
    : CNetTcp()
{
    HOST = host;
    PORT = port;
    TYPE = NetType::TCP_Client;

    RECV_BUF = new unsigned char[BUFFER_SIZE];
    if (RECV_BUF == nullptr)
    {
        throw CNetException("new recv buffer failed", errno);
    }
    SEND_BUF = new unsigned char[BUFFER_SIZE];
    if (SEND_BUF == nullptr)
    {
        throw CNetException("new send buffer failed", errno);
    }
}

CNetTcpClientWithBuffer::CNetTcpClientWithBuffer(const int socket_fd,
                                                 const string &host, const int port)
    : CNetTcp()
{
    FD = socket_fd;
    HOST = host;
    PORT = port;
    TYPE = NetType::TCP_Client;

    RECV_BUF = new unsigned char[BUFFER_SIZE];
    if (RECV_BUF == nullptr)
    {
        throw CNetException("new recv buffer failed", errno);
    }
    SEND_BUF = new unsigned char[BUFFER_SIZE];
    if (SEND_BUF == nullptr)
    {
        throw CNetException("new send buffer failed", errno);
    }
}

CNetTcpClientWithBuffer::~CNetTcpClientWithBuffer()
{
    if (RECV_BUF)
    {
        delete[] RECV_BUF;
        RECV_BUF = nullptr;
    }
    if (SEND_BUF)
    {
        delete[] SEND_BUF;
        SEND_BUF = nullptr;
    }
}

int CNetTcpClientWithBuffer::GetClientPort()
{
    struct sockaddr_in client;
    int len = sizeof(client);
    int ret = ::getsockname(
        FD,
        (struct sockaddr *)&client,
        (socklen_t *)&len);
    // return ntohs(client.sin_port);
    return client.sin_port;
}

void CNetTcpClientWithBuffer::Connect()
{
    if (FD > 0)
    {
        throw CNetException("already connected", 0);
    }

    FD = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (FD < 0)
    {
        throw CNetException("Create socket failed" + string(LAST_ERROR), errno);
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);

    int ret = ::connect(FD, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        throw CNetException("Connect server with error:" + string(LAST_ERROR), errno);
    }

    if (!this->SetNonBlock())
    {
        this->Close();
        throw CNetException("Set nonblock failed:" + string(LAST_ERROR), errno);
    }
}

void CNetTcpClientWithBuffer::Reconnect()
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);

    int ret = ::connect(FD, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        throw CNetException("Connect server with error:" + string(LAST_ERROR), errno);
    }

    if (!this->SetNonBlock())
    {
        this->Close();
        throw CNetException("Set nonblock failed:" + string(LAST_ERROR), errno);
    }
}

int CNetTcpClientWithBuffer::RecvFromSocket()
{
    if (FD < 0)
    {
        throw CNetException("socket is not connected", CNetException::ERR_InvalidParam);
    }
    if (RECV_BUF == nullptr)
    {
        throw CNetException("recv buffer is null", CNetException::ERR_RecvBufFull);
    }

    // 如果RECV_BUF已满
    if (RECV_BUF_CNT >= BUFFER_SIZE)
    {
        return RECV_BUF_CNT;
    }

    int recv_bytes = ::recv(FD, RECV_BUF + RECV_BUF_CNT, BUFFER_SIZE - RECV_BUF_CNT, 0);
    if (likely(recv_bytes > 0))
    {
        RECV_BUF_CNT += recv_bytes;
        return RECV_BUF_CNT;
    }
    else if (recv_bytes == 0)
    {
        Close();
        throw CNetException("socket is closed by peer", CNetException::ERR_SocketClosed);
    }
    else // < 0
    {
        // 异常情况
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            // 当前内核缓冲区无可读数据， 需要等待
            return RECV_BUF_CNT;
        }
        else if (errno == EINTR)
        {
            // 信号中断, 需要重试
            return RECV_BUF_CNT;
        }
        else
        {
            string str = "recv failed " + string(LAST_ERROR);
            throw CNetException(str, CNetException::ERR_RecvFailed);
        }
    }
}
int CNetTcpClientWithBuffer::SendToSocket()
{
    if (FD < 0)
    {
        throw CNetException("socket is not connected", CNetException::ERR_InvalidParam);
    }
    if (SEND_BUF == nullptr)
    {
        throw CNetException("send buffer is null", CNetException::ERR_SendBufFull);
    }

    if (SEND_BUF_CNT <= 0)
    {
        // 发送缓冲区为空
        return 0;
    }

    int send_bytes = ::send(FD, SEND_BUF, SEND_BUF_CNT, MSG_NOSIGNAL | MSG_DONTWAIT);
    if (0 == send_bytes)
    { // socket is closed by peer
        Close();
        throw CNetException("socket is closed by peer", CNetException::ERR_SocketClosed);
    }
    else if (likely(send_bytes > 0))
    {
        SEND_BUF_CNT -= send_bytes;
        if (SEND_BUF_CNT > 0)
        {
            memmove(SEND_BUF, SEND_BUF + send_bytes, SEND_BUF_CNT);
        }
        return send_bytes;
    }
    else // < 0
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            // 当前内核缓冲区满， 需要等待
            LOG_WARN("send failed %s(%d). need retry.", LAST_ERROR, errno);
            return 0;
        }
        else if (errno == EINTR)
        {
            // 信号中断, 需要重试
            LOG_WARN("send failed %s(%d). need retry.", LAST_ERROR, errno);
            return 0;
        }
        else
        {
            string str = "send failed " + string(LAST_ERROR);
            throw CNetException(str, CNetException::ERR_SendFailed);
        }
    }
}

vector<unsigned char> CNetTcpClientWithBuffer::get()
{
    vector<unsigned char> ret;
    if (RECV_BUF == nullptr)
        return ret;

    if (RECV_BUF_CNT < 1)
    {
        return ret;
    }
    
    for (int i=0; i<RECV_BUF_CNT; i++)
    {

    }
    ret.resize(RECV_BUF_CNT);
    memcpy(&ret[0], RECV_BUF, RECV_BUF_CNT);
    memset(RECV_BUF, 0, BUFFER_SIZE);
    RECV_BUF_CNT = 0;
    return ret;
}

int CNetTcpClientWithBuffer::put(const unsigned char *data, const int data_size)
{
    if (SEND_BUF == nullptr)
    {
        throw CNetException("send buffer is null", CNetException::ERR_InvalidParam);
    }

    if (data_size > BUFFER_SIZE)
    {
        const size_t newSize = (data_size / 1024 + 1) * 1024;
        ExpandBuffer(newSize);
    }

    if ((BUFFER_SIZE - SEND_BUF_CNT) < data_size)
    {
        return 0;
    }

    memcpy(SEND_BUF + SEND_BUF_CNT, data, data_size);
    SEND_BUF_CNT += data_size;
    return data_size;
}

int CNetTcpClientWithBuffer::put(const std::vector<char> &data)
{
    return this->put(&data[0], data.size());
}

void CNetTcpClientWithBuffer::ExpandBuffer(const size_t &newSize)
{
    if (!RECV_BUF || !SEND_BUF)
    {
        return;
    }

    if (newSize <= BUFFER_SIZE)
    {
        return;
    }

    unsigned char *newRecvBuf = new unsigned char[newSize];
    unsigned char *newSendBuf = new unsigned char[newSize];
    if (!newRecvBuf || !newSendBuf)
    {
        if (newRecvBuf)
        {
            delete[] newRecvBuf;
        }
        if (newSendBuf)
        {
            delete[] newSendBuf;
        }
        return;
    }

    memcpy(newRecvBuf, RECV_BUF, BUFFER_SIZE);
    memcpy(newSendBuf, SEND_BUF, BUFFER_SIZE);

    delete[] RECV_BUF;
    delete[] SEND_BUF;

    LOG_WARN("Expand buffer from %d to %d", BUFFER_SIZE, newSize);
    RECV_BUF = newRecvBuf;
    SEND_BUF = newSendBuf;
    BUFFER_SIZE = newSize;
}
