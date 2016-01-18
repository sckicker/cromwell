#include "acceptor.h"

#include <errno.h>
#include <fcntl.h>

#include "event_loop.h"
#include "channel.h"
#include "socket.h"
#include "socket_opt.h"

namespace cromwell {

Acceptor::Acceptor(EventLoop& loop, const char* ip, int port, bool reuseport)
  : loop_(loop),
  accept_socket_(-1),
  acc{
}

}//end-cromwell.
