#ifndef SERVER_H
#define SERVER_H

#include <errno.h>
#include <functional>
#include <netinet/in.h>
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

namespace network {

class Server {
  static constexpr int REQUESTMAX = 32;
  static constexpr int OFFSET{sizeof(size_t)};

public:
  using RecvCallback = std::function<void(std::vector<char> const &)>;

  Server();
  ~Server();

  bool listen(int port);
  void send(std::string const &message);
  void send(std::vector<char> const &message);
  void send(const char *message, size_t length);
  void setOnRecv(RecvCallback const &cb);
  void close();

private:
  int m_listenSd{-1};
  std::thread m_listen_t;
  RecvCallback m_rvCb;
  std::vector<pollfd> m_fds;

  bool write(int sd, const char *__buffer, size_t __buffersize);
  int read(int sockfd, bool &err);
  void recv();
};

} // namespace network

} // namespace rpx

#endif // SERVER_H
