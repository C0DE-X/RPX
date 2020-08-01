#pragma once

#include <functional>
#include <rpx/ICommunication.h>
#include <string>
#include <thread>
#include <vector>

namespace rpx {

namespace communication {

class TCPClient : public ICommunication {

public:
  using RecvCallback = std::function<void(std::vector<char> const &)>;

  enum AF_TYPE { AF_IPV4, AF_IPV6 };

  TCPClient() = default;
  ~TCPClient();

  bool connect(std::string addr, int port, AF_TYPE type = AF_IPV6);
  void close();

  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  int m_sockfd{-1};
  std::thread m_recv_t;
  RecvCallback m_rvCb;

  bool write(const char *__buffer, size_t __buffersize);
  bool read();
};

} // namespace communication

} // namespace rpx