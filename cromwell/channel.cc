#include "channel.h"

#include "macros.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

namespace cromwell {

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
  ASSERT(!event_handling_);
  ASSERT(!add_to_loop_);
  if (loop_.IsInLoopThread()) {
    ASSERT(!loop_.HaveChannel(this));
  }
}

}//end-cromwell
