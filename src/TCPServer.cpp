#include <errno.h>
#include <netinet/in.h>
#include <rpx/TCPServer.h>
#include <rpx/Utils.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace rpx {

namespace communication {

namespace server {

int _listen(int __fd, int __n) { return listen(__fd, __n); }

size_t _write(int __fd, const void *__buf, size_t __n) {
  signal(SIGPIPE, SIG_IGN);
  return write(__fd, __buf, __n);
}

ssize_t _recv(int __fd, void *__buf, size_t __n, int __flags) {
  return recv(__fd, __buf, __n, __flags);
}

void _close(int sockfd) {
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
}

} // namespace server

TCPServer::TCPServer() {}

TCPServer::~TCPServer() {
  close();
  if (m_listen_t.joinable())
    m_listen_t.join();
}

bool TCPServer::listen(int port) {
  int rc, on = 1;
  struct sockaddr_in6 addr;

  /*************************************************************/
  /* Create an AF_INET6 stream socket to receive incoming      */
  /* connections on                                            */
  /*************************************************************/
  m_listenSd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (m_listenSd < 0) {
    perror("socket() failed");
    return false;
  }

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  rc =
      setsockopt(m_listenSd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  if (rc < 0) {
    perror("setsockopt() failed");
    server::_close(m_listenSd);
    return false;
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for      */
  /* the incoming connections will also be nonblocking since   */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  rc = ioctl(m_listenSd, FIONBIO, (char *)&on);
  if (rc < 0) {
    perror("ioctl() failed");
    server::_close(m_listenSd);
    return false;
  }

  /*************************************************************/
  /* Bind the socket                                           */
  /*************************************************************/
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
  addr.sin6_port = htons(port);
  rc = bind(m_listenSd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0) {
    perror("bind() failed");
    server::_close(m_listenSd);
    return false;
  }

  /*************************************************************/
  /* Set the listen back log                                   */
  /*************************************************************/
  rc = server::_listen(m_listenSd, REQUESTMAX);
  if (rc < 0) {
    perror("listen() failed");
    server::_close(m_listenSd);
    return false;
  }

  /*************************************************************/
  /* Set up the initial listening socket                        */
  /*************************************************************/
  m_fds.push_back({m_listenSd, POLLIN, 0});

  m_listen_t = std::thread([this]() { this->recv(); });

  return true;
}

bool TCPServer::send(rpx::bytearray const &message) {
  bool ret = false;
  for (auto &sd : m_fds)
    if (sd.fd > 0 && sd.fd != m_listenSd) {
      ret |= write(sd.fd, message.data(), message.size());
    }

  return ret;
}

void TCPServer::setOnRecv(const TCPServer::RecvCallback &cb) { m_rvCb = cb; }

void TCPServer::close() { server::_close(m_listenSd); }

bool TCPServer::write(int sd, const char *__buffer, size_t __buffersize) {
  size_t totalBytes{0};
  auto size = rpx::Utils::fromSize(__buffersize);
  std::vector<char> buffer(__buffersize + size.size());
  memcpy(buffer.data(), size.data(), size.size());
  memcpy(buffer.data() + size.size(), __buffer, __buffersize);

  while (totalBytes < buffer.size()) {
    int sc = server::_write(sd, buffer.data() + totalBytes,
                            buffer.size() - totalBytes);
    if (sc <= 0) {
      perror("  write() failed");
      return false;
    }

    totalBytes -= sc;
  }
  return true;
}

int TCPServer::read(int sockfd, bool &err) {

  int rc = 0;
  size_t totalBytes = 0;
  std::array<char, 8> sizeBuffer;

  while (totalBytes < sizeBuffer.size()) {
    rc = server::_recv(sockfd, sizeBuffer.data(), sizeBuffer.size(), MSG_PEEK);
    if (rc <= 0) {
      if (errno != EWOULDBLOCK) {
        // perror("  recv() failed");
        err = true;
      }
      return rc;
    }
    totalBytes += rc;
  }

  size_t msgSize = rpx::Utils::toSize(sizeBuffer) + sizeBuffer.size();
  std::vector<char> buffer(msgSize);
  totalBytes = 0;

  while (totalBytes < buffer.size()) {
    rc = server::_recv(sockfd, buffer.data() + totalBytes,
                       buffer.size() - totalBytes, 0);
    if (rc <= 0) {
      if (errno != EWOULDBLOCK) {
        // perror("  recv() failed");
        err = true;
      }
      return rc;
    }
    totalBytes += rc;
  }

  // remove OFFSET from
  buffer.erase(buffer.begin(), buffer.begin() + sizeBuffer.size());

  if (m_rvCb)
    m_rvCb(buffer);

  return msgSize;
}

void TCPServer::recv() {
  int rc;
  int new_sd = -1;
  int end_server = false;
  bool close_conn;

  /*************************************************************/
  /* Loop waiting for incoming connects or for incoming data   */
  /* on any of the connected sockets.                          */
  /*************************************************************/
  do {
    /***********************************************************/
    /* Call poll() for it to complete.      */
    /***********************************************************/
    // printf("Waiting on poll()...\n");
    rc = poll(m_fds.data(), m_fds.size(), -1);

    /***********************************************************/
    /* Check to see if the poll call failed.                   */
    /***********************************************************/
    if (rc < 0) {
      perror("  poll() failed");
      break;
    }

    /***********************************************************/
    /* One or more descriptors are readable.  Need to          */
    /* determine which ones they are.                          */
    /***********************************************************/
    for (auto sock = m_fds.begin(); sock != m_fds.end();) {
      /*********************************************************/
      /* Loop through to find the descriptors that returned    */
      /* POLLIN and determine whether it's the listening       */
      /* or the active connection.                             */
      /*********************************************************/

      if (sock->revents == 0) {
        ++sock;
        continue;
      }

      /*********************************************************/
      /* If revents is not POLLIN, it's an unexpected result,  */
      /* log and end the server.                               */
      /*********************************************************/
      if (sock->revents != POLLIN) {
        // printf("  Error! revents = %d\n", sock->revents);
        end_server = true;
        break;
      }

      /*******************************************************/
      /* Listening descriptor is readable.                   */
      /*******************************************************/
      if (sock->fd == m_listenSd) {
        // printf("  Listening socket is readable\n");

        /*******************************************************/
        /* Accept all incoming connections that are            */
        /* queued up on the listening socket before we         */
        /* loop back and call poll again.                      */
        /*******************************************************/
        do {
          /*****************************************************/
          /* Accept each incoming connection. If               */
          /* accept fails with EWOULDBLOCK, then we            */
          /* have accepted all of them. Any other              */
          /* failure on accept will cause us to end the        */
          /* server.                                           */
          /*****************************************************/
          new_sd = accept(m_listenSd, NULL, NULL);
          if (new_sd < 0) {
            if (errno != EWOULDBLOCK) {
              perror("  accept() failed");
              end_server = true;
            }
            ++sock;
            break;
          }

          /*****************************************************/
          /* Add the new incoming connection to the            */
          /* pollfd structure                                  */
          /*****************************************************/
          // printf("  New incoming connection - %d\n", new_sd);
          sock = m_fds.insert(sock, {new_sd, POLLIN, 0});

          /*****************************************************/
          /* Loop back up and accept another incoming          */
          /* connection                                        */
          /*****************************************************/
        } while (new_sd != -1);

        if (end_server)
          sock = m_fds.end();
        else
          ++sock;
      }

      /*********************************************************/
      /* This is not the listening socket, therefore an        */
      /* existing connection must be readable                  */
      /*********************************************************/

      else {
        // printf("  Descriptor %d is readable\n", sock->fd);
        close_conn = false;
        /*******************************************************/
        /* Receive all incoming data on this socket            */
        /* before we loop back and call poll again.            */
        /*******************************************************/

        /*****************************************************/
        /* Receive data on this connection until the         */
        /* recv fails with EWOULDBLOCK. If any other         */
        /* failure occurs, we will close the                 */
        /* connection.                                       */
        /*****************************************************/
        rc = read(sock->fd, close_conn);

        /*****************************************************/
        /* Check to see if the connection has been           */
        /* closed by the client                              */
        /*****************************************************/
        if (rc == 0) {
          // printf("  Connection closed\n");
          close_conn = true;
        }

        /*******************************************************/
        /* If the close_conn flag was turned on, we need       */
        /* to clean up this active connection. This            */
        /* clean up process includes removing the              */
        /* descriptor.                                         */
        /*******************************************************/
        if (close_conn) {
          server::_close(sock->fd);
          sock = m_fds.erase(sock);
        } else {
          ++sock;
        }

      } /* End of existing connection is readable             */
    }   /* End of loop through pollable descriptors              */
  } while (!end_server); /* End of serving running.    */

  /*************************************************************/
  /* Clean up all of the sockets that are open                 */
  /*************************************************************/
  for (auto &sd : m_fds)
    server::_close(sd.fd);
  m_fds.clear();
}

} // namespace communication

} // namespace rpx
