#ifndef _CROMWELL_SOCKET_H
#define _CROMWELL_SOCKET_H

namespace cromwell {

class Socket : std::noncopyable {
public:
    Socket();
    virtual ~Socket();

public:
    bool Valid() const { return fd_ >= 0; }
    int Socketfd() {return fd_;}

public:
    void Close();

    bool Connect(const char* ip, uint16_t port, const char* bind);
    bool Listen(const char* ip, uint16_t port);

    int Accept();

    bool Send(const char* data, int data_len);
    bool Recv(char* buf, int buf_len);

    void SetTcpNoDeday(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);
    
private:
    int fd_;
};

}//end-namespace cromwell.

#endif
