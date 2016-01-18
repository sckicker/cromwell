#include "connector.h"

#include "event_loop.h"
#include "socket_opt.h"

#include <errno.h>

namespace cromwell {

const int Connector::kMaxRetryDelayMs = 30 * 1000;
const int Connector::kInitRetryDelayMs = 500;

Connector::Connector(EventLoop& loop, const char* ip, int port)
  : loop_(loop),
  port_(port),
  connect_(false),
  state_(kDisconnect) {
  strncpy(server_ip_, 16, ip);
}

Connector::~Connector() {
  //To-Do.
}

void Connector::Start() {
  connect_ = true;
  loop_.Running(std::bind(&Connector::StartInLoop, this));
}

//Action func for start.
void Connector::StartInLoop() {
  loop_.AssertInLoopThread();
  assert(state_ == kDisconnect);
  if (connect_) {
    this->Connect();
  }else {
    //To-Do.
  }
}

void Connector::Stop() {
  connect_ = false;
  loop_.QueueInLoop(std::bind(&Connector::StopInLoop, this));
  //FIXME: unsafe, cancel time.
}

// Action Func for stop.
void Connector::StopInLoop() {
  loop_.AssertInLoopThread();
  if (state_ == kConnecting) {
    SetState(kDisconnect);
    int sockfd = this->RemoveAndResetChannel();
    Retry(sockfd);
  }
}

void Connector::Connect() {
  int sockfd = tcp_connect(nullptr, server_ip_, server_port_);
  //to-do.
}

void Connector::Restart() {
  loop_.AssertInLoopThread();
  SetState(kDisconnect);
  connect_ = true;
  StartInLoop();
}

void Connector::Connecting() {
  SetState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_.SetWriteCallback(std::bind(&Connector::HandleWrite, this));
  channel_.EnableWriting();
}

int Connector::RemoveAndResetChannel() {
  channel_.DisableAll();
  channel_.Remove();
  int sockfd = channel_.fd();
  loop_.QueueInLoop(std::bind(&Connector::ResetChannel, this));
  return sockfd;
}

void Connector::ResetChannel() {
  channel_.Reset();
}

void Connector::HandleWrite() {
  if (state_ == kConnecting) {
    int sockfd = this->RemoveAndResetChannel();
    this->SetState(kConnected);
    if (connect_) {
      this->NewConnectionCallback(sockfd);
    }else {
      close(sockfd);
    }
  }else {
    assert(state_ == kDisconnect);
  }
}

void Connector::HandleError() {
  if (state_ == kConnecting) {
    int sockfd = this->RemoveAndResetChannel();
    int err = GetSocketError(sockfd);
    this->Retry(sockfd);
  }
}

void Connector::Retry(int sockfd) {
  close(sockfd);
  SetState(kDisconnect);
  if (connect_) {
    loop_.RunAfter(retry_delay_ms_ / 1000, std::bind(&Connector::StartInLoop, shared_from_this()));
    retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);
  }
}

}//end-cromwell.
