#pragma once

#include <functional>
#include <rpx/ICommunication.h>
#include <poll.h>
#include <thread>

namespace rpx::communication {

class TCPServer : public ICommunication {
  static constexpr int REQUESTMAX = 32;

public:
  using RecvCallback = std::function<void(std::vector<char> const &)>;

  TCPServer();
  ~TCPServer();

  bool listen(int port);
  void close() const;

  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  int m_listenSd{-1};
  std::thread m_listen_t;
  RecvCallback m_rvCb;
  std::vector<pollfd> m_fds;

  static bool write(int sd, const char *wbuffer, size_t wbuffersize);
  int read(int sockfd, bool &err);
  void recv();
};

} // namespace rpx::communication
