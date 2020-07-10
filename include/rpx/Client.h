#ifndef CLIENT_H
#define CLIENT_H

#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace rpx {

namespace network {

class Client {
  static constexpr int OFFSET{sizeof(size_t)};

public:
  using RecvCallback = std::function<void(std::vector<char> const &)>;

  enum AF_TYPE { AF_IPV4, AF_IPV6 };

  Client() = default;
  ~Client();

  bool connect(std::string addr, int port, AF_TYPE type = AF_IPV6);
  void close();

  bool send(std::string const &message);
  bool send(std::vector<char> const &message);
  bool send(const char *message, size_t length);
  void setOnRecv(RecvCallback const &cb);

private:
  int m_sockfd{-1};
  std::thread m_recv_t;
  RecvCallback m_rvCb;

  bool write(const char *__buffer, size_t __buffersize);
  bool read();
};

} // namespace network

} // namespace rpx

#endif // CLIENT_H
