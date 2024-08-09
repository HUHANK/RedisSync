#pragma once
#include <string>
#include <memory>
#include <vector>
#include <exception>
#include "log/mflog.h"

// 网络类型（TCP/UDP）
enum class NetType
{
    TCP_Client, // TCP 客户端
    TCP_Server, // TCP 服务端
    NONE,
};

// 网络异常
class CNetException : public std::exception
{
public:
    enum
    {
        ERR = -1,
        ERR_InvalidParam,
        ERR_RecvBufFull,
        ERR_SendBufFull,
        ERR_SocketClosed,
        ERR_RecvFailed,
        ERR_SendFailed,
    };

public:
    CNetException() = default;
    CNetException(const std::string &msg, const unsigned int code)
        : error_code(code), error_msg(msg) {}

    const char *what() const noexcept override
    {
        const std::string msg = "(" + std::to_string(error_code) + ") " + error_msg;
        return msg.c_str();
    }

private:
    unsigned int error_code = ERR;
    std::string error_msg = "";
};

// TCP 基类
class CNetTcp
{
public:
    explicit CNetTcp();
    virtual ~CNetTcp();

    virtual void Close();

    bool SetNonBlock();
    bool SetAddrReuse();
    bool SetRecvBufSize(const unsigned long size);
    bool SetSendBufSize(const unsigned long size);

    std::string to_string() const;
    int fd() const { return FD; }
    NetType type() const { return TYPE; }
    int port() const { return PORT; }
    std::string host() const { return HOST; }

protected:
    std::string HOST = "127.0.0.1";
    int PORT = 0;
    int FD = -1;
    NetType TYPE;
};

class CNetTcpClientWithBuffer : public CNetTcp
{ // NONE BLOCK TCP
public:
    CNetTcpClientWithBuffer(const std::string &host, const int port);
    CNetTcpClientWithBuffer(const int socket_fd, const std::string &host = "", const int port = 0);
    ~CNetTcpClientWithBuffer();

public:
    void Connect();
    void Reconnect();

    int GetClientPort();

    // 从Socket读数据到缓冲区中
    int RecvFromSocket();
    // 把缓冲区中的数据发送出去
    int SendToSocket();

    // 从缓冲区中获取数据
    std::vector<unsigned char> get();
    // 把指定的数据放到缓冲区中，不代表实际发送出去
    int put(const unsigned char *data, const int data_size);
    int put(const std::vector<char> &data);

    bool CanRead() { return RECV_BUF_CNT > 0; }
    bool CanWrite() { return SEND_BUF_CNT > 0; }
private:
    // 扩充收发缓冲区大小至指定大小
    void ExpandBuffer(const size_t &newSize);

private:
    // BUFFER_SIZE表示RECV_BUF和SEND_BUF的最大缓冲区大小，
    // 这不是最终值，会随着实际包的大小动态调整这个值，以满足实际需求
    size_t BUFFER_SIZE = 1024 * 1024 * 2;
    unsigned char *RECV_BUF = nullptr;
    unsigned char *SEND_BUF = nullptr;
    int RECV_BUF_CNT = 0;
    int SEND_BUF_CNT = 0;
};

class CNetTcpServer : public CNetTcp
{
public:
    CNetTcpServer(const int port);
    ~CNetTcpServer();

public:
    bool InitServer();

    std::shared_ptr<CNetTcpClientWithBuffer> Accept();
};
