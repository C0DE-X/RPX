#include <rpx/Client.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

namespace rpx {

namespace network {

namespace client {

int _connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
  return connect(__fd, __addr, __len);
}

void _close(int sockfd) {
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
}

size_t _write(int __fd, const void *__buf, size_t __n) {
  signal(SIGPIPE, SIG_IGN);
  return write(__fd, __buf, __n);
}

} // namespace client

Client::~Client() {
  close();
  if (m_recv_t.joinable())
    m_recv_t.join();
}

bool Client::connect(std::string addr, int port, AF_TYPE type) {

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
    struct sockaddr_in serv_addr4;
    struct hostent *server;

    bzero((char *)&serv_addr4, sizeof(serv_addr4));
    server = gethostbyname(addr.c_str());
    if (server == NULL) {
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
    struct sockaddr_in6 serv_addr6;

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
    ;
  });
  return true;
}

void Client::close() { client::_close(m_sockfd); }

bool Client::send(const std::string &message) {
  return write(message.c_str(), message.size());
}

bool Client::send(const std::vector<char> &message) {
  return write(message.data(), message.size());
}

bool Client::send(const char *message, size_t length) {
  return write(message, length);
}

void Client::setOnRecv(const RecvCallback &cb) { m_rvCb = cb; }

bool Client::write(const char *__buffer, size_t __buffersize) {

  if (m_sockfd < 0)
    return false;

  size_t totalBytes{0};
  std::vector<char> buffer(__buffersize + OFFSET);
  memcpy(buffer.data(), &__buffersize, OFFSET);
  memcpy(buffer.data() + OFFSET, __buffer, __buffersize);

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

bool Client::read() {

  int rc = 0;
  size_t msgSize = 0, totalBytes = 0;

  while (totalBytes < OFFSET) {
    rc = recv(m_sockfd, reinterpret_cast<char *>(&msgSize), OFFSET, MSG_PEEK);
    if (rc <= 0) {
      if (errno != EWOULDBLOCK) {
        // perror("  recv() failed");
        return false;
      }
      return true;
    }
    totalBytes += rc;
  }

  msgSize += sizeof(size_t);
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
  buffer.erase(buffer.begin(), buffer.begin() + OFFSET);

  if (m_rvCb)
    m_rvCb(buffer);

  return true;
}

} // namespace network

} // namespace rpx
