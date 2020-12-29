#include <rpx/TCPClient.h>
#include <rpx/Utils.h>

#include <netdb.h>
#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>


namespace rpx::communication {

namespace client {

int _connect(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len) {
  return connect(fd, addr, len);
}

void _close(int sockfd) {
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
}

size_t _write(int fd, const void *buf, size_t n) {
  signal(SIGPIPE, SIG_IGN);
  return write(fd, buf, n);
}

} // namespace client

TCPClient::~TCPClient() {
  close();
  if (m_recv_t.joinable())
    m_recv_t.join();
}

bool TCPClient::connect(const std::string& addr, int port, AF_TYPE type) {

  // create socket
  if (type == AF_TYPE::AF_IPV4)
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  else if (type == AF_TYPE::AF_IPV6)
    m_sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

  if (m_sockfd < 0) {
    perror("ERROR opening socket\n");
    return false;
  }

  // set socket blocking
  int on{0};
  int rc = ioctl(m_sockfd, FIONBIO, (char *)&on);
  if (rc < 0) {
    perror("ioctl() failed");
    client::_close(m_sockfd);
    return false;
  }

  // connect ipv4
  if (type == AF_TYPE::AF_IPV4) {
    struct sockaddr_in serv_addr4{};
    struct hostent *server;

    bzero((char *)&serv_addr4, sizeof(serv_addr4));
    server = gethostbyname(addr.c_str());
    if (server == nullptr) {
      perror("ERROR, no such host\n");
      return false;
    }
    serv_addr4.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr4.sin_addr.s_addr,
          server->h_length);
    serv_addr4.sin_port = htons(port);
    if (client::_connect(m_sockfd, reinterpret_cast<sockaddr *>(&serv_addr4),
                         sizeof(serv_addr4)) < 0) {
      perror("ERROR connecting");
      return false;
    }
  }

  // connect ipv6
  if (type == AF_TYPE::AF_IPV6) {
    struct sockaddr_in6 serv_addr6{};

    bzero((char *)&serv_addr6, sizeof(serv_addr6));
    serv_addr6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, addr.c_str(), &serv_addr6.sin6_addr);
    serv_addr6.sin6_port = htons(port);
    if (client::_connect(m_sockfd, reinterpret_cast<sockaddr *>(&serv_addr6),
                         sizeof(serv_addr6)) < 0) {
      perror("ERROR connecting");
      return false;
    }
  }

  m_recv_t = std::thread([this]() {
    while (read())
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  });
  return true;
}

void TCPClient::close() const { client::_close(m_sockfd); }

bool TCPClient::send(rpx::bytearray const &message) {
  return write(message.data(), message.size());
}

void TCPClient::setOnRecv(RecvCallback const &cb) { m_rvCb = cb; }

bool TCPClient::write(const char *wbuffer, size_t wbuffersize) const {

  if (m_sockfd < 0)
    return false;

  size_t totalBytes{0};
  auto size = rpx::Utils::fromSize(wbuffersize);
  std::vector<char> buffer(wbuffersize + size.size());
  memcpy(buffer.data(), size.data(), size.size());
  memcpy(buffer.data() + size.size(), wbuffer, wbuffersize);

  while (totalBytes < buffer.size()) {
    int sc = client::_write(m_sockfd, buffer.data() + totalBytes,
                            buffer.size() - totalBytes);
    if (sc <= 0) {
      perror("  write() failed");
      return false;
    }

    totalBytes -= sc;
  }
  return true;
}

bool TCPClient::read() {

  int rc;
  size_t totalBytes = 0;
  std::array<char, 8> sizeBuffer{};

  while (totalBytes < sizeBuffer.size()) {
    rc = recv(m_sockfd, sizeBuffer.data(), sizeBuffer.size(), MSG_PEEK);
    if (rc <= 0) {
      if (errno != EWOULDBLOCK) {
        // perror("  recv() failed");
        return false;
      }
      return true;
    }
    totalBytes += rc;
  }

  size_t msgSize = rpx::Utils::toSize(sizeBuffer) + sizeBuffer.size();
  std::vector<char> buffer(msgSize);
  totalBytes = 0;

  while (totalBytes < buffer.size()) {
    rc = recv(m_sockfd, buffer.data() + totalBytes, buffer.size() - totalBytes,
              0);
    if (rc <= 0) {
      if (errno != EWOULDBLOCK) {
        // perror("  recv() failed");
        return false;
      }
      return false;
    }
    totalBytes += rc;
  }

  // remove OFFSET from
  buffer.erase(buffer.begin(), buffer.begin() + sizeBuffer.size());

  if (m_rvCb)
    m_rvCb(buffer);

  return true;
}

} // namespace rpx::communication
