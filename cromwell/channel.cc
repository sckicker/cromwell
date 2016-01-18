#include "channel.h"

#include "macros.h"

namespace cromwell {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop& loop, int fd)
: loop_(loop),
  fd_(fd),
  events_(0),
  revents_(0),
  index_(-1),
  log_hup_(true),
  tied_(false),
  event_handling_(false),
  add_to_loop_(false) {

  }

Channel::~Channel() {
  assert(!event_handling_);
  assert(!add_to_loop_);
  if (loop_.IsInLoopThread()) {
    assert(!loop_.HaveChannel(this));
  }
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::Update() {
  add_to_loop_ = true;
  loop_.UpdateChannel(this);
}

void Channel::Remove() {
  assert(IsNoneEvent());
  add_to_loop_ = false;
  loop_.RemoveChannel(this);
}

void Channel::HandleEvent(Timestamp receive_time) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(receive_time);
    }
  }else {
    HandleEventWithGuard(receive_time);
  }
}

void Channel::HandleEventWithGuard(Timestamp receive_time) {
  event_handling = true;
  if ((revents_ & POLLHUP) && !(reevents_ & POLLIN)) {
    if (log_hup_) {}
    if (close_cb_) close_cb_();
  }

  if (revents_ & POLLNVAL) {
    LOG_WARN("Channel::handle_event POLLNVAL");
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_cb_) error_cb_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_cb_) read_cb_();
  }
  if (revents_ & POLLOUT) {
    if (write_cb_) write_cb_();
  }

  event_handling_ = false;
}

std::string Channel::ReventsToString() const {
  std::ostringstream oss;
  oss << fd_ << ": ";
  if (revents_ & POLLIN) oss << "IN ";
  if (revents_ & POLLPRI) oss << "PRI ";
  if (revents_ & POLLOUT) oss << "OUT ";
  if (revents_ & POLLHUP) oss << "HUP ";
  if (revents_ & POLLRDHUP) oss << "RDHUP ";
  if (revents_ & POLLERR) oss << "ERR ";
  if (revents_ & POLLNVAL) oss << "NVAL ";

  return oss.str();
}

}//end-cromwell
