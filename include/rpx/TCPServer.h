#pragma once

#include <errno.h>
#include <functional>
#include <netinet/in.h>
#include <rpx/ICommunication.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <thread>
#include <vector>

namespace rpx {

namespace communication {

class TCPServer : public ICommunication {
  static constexpr int REQUESTMAX = 32;

public:
  using RecvCallback = std::function<void(std::vector<char> const &)>;

  TCPServer();
  ~TCPServer();

  bool listen(int port);
  void close();

  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  int m_listenSd{-1};
  std::thread m_listen_t;
  RecvCallback m_rvCb;
  std::vector<pollfd> m_fds;

  bool write(int sd, const char *__buffer, size_t __buffersize);
  int read(int sockfd, bool &err);
  void recv();
};

} // namespace communication

} // namespace rpx
