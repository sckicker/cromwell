#include "socket.h"
#include "socket_opt.h"

namespace cromwell {

Socket::Socket():
  : fd_(-1) {
  	fd_ = create_socket(nullptr, AF_INET);
}

Socket::~Socket() {

}

bool Socket::Listen(const char* ip, uint16_t port) {

}

}//end cromwell.
