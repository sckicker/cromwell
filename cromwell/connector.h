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

  Connector();
};

}//end-cromwell.

#endif
