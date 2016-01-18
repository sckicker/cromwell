#ifndef _CROMWELL_CONNECTOR_H
#define _CROMWELL_CRNNECTOR_H

#include <functional>
#include <memory>

namespace cromwell {

class Channel;
class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector> {
public:
  typedef std::function<void(int sockfd)> NewConnectionCallback;

  Connector(EventLoop& loop, const char* ip, int port);
  ~Connector();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    new_conn_cb_ = cb;
  }

  void Start();
  void Restart();
  void Stop();

  const char* ServerIp() const { return server_ip_; }
  int ServerPort const { return server_port_; }

private:
  enum ConnectorState { kDisconnect, kConnecting, kConnected, }

  void SetState(ConnectorState s) { state_ = s; }
  void StartInLoop();
  void StopInLoop();
  void Connect();
  void Connecting(int sockfd);
  void HandleWrite();
  void HandleError();
  void Retry(int sockfd);
  int RemoveAndResetChannel();
  void ResetChannel();

private:
  static const int kMaxRetryDelayMs;
  static const int kInitRetryDelayMs;

private:
  EventLoop& loop_;
  char server_ip_[16];
  int server_port_;
  ConnectorState state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_conn_cb_;
};

}//end-cromwell.

#endif
