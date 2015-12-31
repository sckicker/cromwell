#ifndef __SOCKET_H
#define __SOCKET_H

namespace cromwell {

class Socket {
public:
    Socket(int sock_type);
    virtual ~Socket();

public:
    bool Valid() const { return fd_ >= 0; }
    int fd() {return fd_;}

public:
    void Close();

    bool Connect(const char* ip, uint16_t port, const char* bind);
    bool Listen(const char* ip, uint16_t port);

    int Accept();

    bool Send(const char* data, int data_len);
    bool Recv(char* buf, int buf_len);

private:
    int sock_type_;
    int fd_;
};

}//end-namespace cromwell.

#endif
