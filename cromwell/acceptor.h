#ifndef _CROMWELL_ACCEPTOR_H
#define _CROMWELL_ACCEPTOR_H

#include <functional>

namespace cromwell {

class EventLoop;
class Channel;
class Socket;

class Acceptor : std::noncopyable {
public:
  typedef std::function<void (sockfd, const char* ip, int port)> NewConnectionCallback;

  Acceptor(EventLoop& loop, const char* ip, int port, bool reuseport);
  ~Acceptor();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    new_conn_cb_ = cb;
  }

  bool Listening() const { return listening_; }
  void Listen();

private:
  void HandleRead();

private:
  EventLoop& loop_;
  Socket accept_socket_;
  Channel accept_channal_;
  NewConnectionCallback new_conn_cb_;
  bool listening_;
  int idle_fd_;
};

}//end-cromwell.

#endif
