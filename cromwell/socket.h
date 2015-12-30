#pragma once

namespace cromwell {

class Socket {
public:
    Socket();
    virtual ~Socket();

public:
    int fd() {return fd_;}

public:
    void Close();

    bool Connect(const char* ip, uint16_t port, const char* bind);
    bool Listen(const char* ip, uint16_t port);

    bool Accept(char** addr);

    void ShutdownWrite();

    void SetTcpNoDelay(bool on);

    void SetReuseAddr(bool on);

    void SetReusePort(bool on);

    void SetKeepAlive(bool on);
private:
    int fd_;
};

}//namespace cromwell.
