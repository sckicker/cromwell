#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <function>
#include <memory>

namespace cromwell {

class Channel : std::noncopyable {
public:
  typedef std::function<void(void)> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop& loop, int fd);
  ~Channel();

  void HandleEvent(Timestamp receive_time);
  void SetReadCallback(const ReadEventCallback& cb) {
    read_cb_ = cb;
  }
  void SetWriteCallback(const EventCallback& cb) {
    write_cb_ = cb;
  }
  void SetCloseCallback(const EventCallback& cb) {
    close_cb_ = cb;
  }
  void SetErrorCallback(const EventCallback& cb) {
    error_cb_ = cb;
  }

  void Tie(const std::shared_ptr<void>&);
  int fd() const { return fd_; }
  int events() const { return events_; }
  bool IsNoneEvent() const { return events_ == kNoneEvent; }

  void EnableReading() const { events_ |= kReadEvent; Update(); }
  void DisableReading() const { events_ &= ~kReadEvent; Update(); }
  void EnableWriting() const { events_ |= kWriteEvent; Update(); }
  void DisableReading() const { events_ &= ~kWriteEvent; Update(); }
  void DisableAll() const { events_ = kNoneEvent; Update(); }
  bool IsWriting() const { return events_ & kWriteEvent; }

  //For poll.
  int Index() const { return index_; }
  void SetIndex(int index) { index_ = index; }

  void DoNotLogHup() { log_hup_ = false; }

  EventLoop& OwnerLoop() { return loop_; }
  void Remove();

private:
  void Update();
  void HandleEventWithGuard(Timestamp receive_time);

private:
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

private:
  EventLoop& loop_;
  const int fd_;
  int events_;
  int revents_;
  int index_;
  bool log_hup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool event_handling_;
  bool add_to_loop_;

  ReadEventCallback read_cb_;
  EventCallback write_cb_;
  EventCallback close_cb_;
  EventCallback error_cb_;
};

}//end-cromwell.

#endif
