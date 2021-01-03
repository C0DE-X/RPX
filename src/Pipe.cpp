#include "rpx/Pipe.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <utility>
#include <thread>

namespace rpx::communication {

int pipe_open(std::string const &path) {
  return open(path.c_str(), O_RDWR | O_NONBLOCK);
}

ssize_t pipe_write(int fd, const void *buf, size_t n) {
  return write(fd, buf, n);
}

ssize_t pipe_read(int fd, void *buf, size_t nbytes) {
  return read(fd, buf, nbytes);
}

int pipe_close(int fd) {
  return close(fd);
}

Pipe::Pipe(std::string path)
    : m_path(std::move(path)) {
  mkfifo(m_path.c_str(), (S_IWUSR | S_IRUSR) | (S_IWGRP | S_IRGRP) | (S_IWOTH | S_IROTH));
  m_fifoHandle = pipe_open(m_path);
}
Pipe::Pipe(const Pipe &other)
    : m_path(other.m_path), m_fifoHandle(pipe_open(other.m_path)) {
}
Pipe::Pipe(Pipe &&other) noexcept
    : m_path(std::move(other.m_path)), m_fifoHandle(pipe_open(m_path)) {
  other.close();
}
Pipe::~Pipe() {
  close();
}
Pipe &Pipe::operator=(const Pipe &other) {
  close();
  m_path = other.m_path;
  m_fifoHandle = pipe_open(m_path);
  return *this;
}
Pipe &Pipe::operator=(Pipe &&other) noexcept {
  close();
  other.close();
  m_path = other.m_path;
  other.m_path.clear();
  m_fifoHandle = pipe_open(m_path);
  return *this;
}
std::vector<char> Pipe::read(bool blocking) const {
  std::vector<char> buffer;
  int bytes = 0, num;

  while (m_fifoHandle >= 0) {
    num = pipe_read(m_fifoHandle, &bytes, sizeof(bytes));
    if (num < 0) {
      if (blocking) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      return buffer;
    }
    buffer.resize(bytes);
    break;
  }

  while (bytes > 0) {
    num = pipe_read(m_fifoHandle, buffer.data() + (buffer.size() - bytes), bytes);
    if (num < 0 || m_fifoHandle < 0) {
      buffer.clear();
      return buffer;
    }
    bytes -= num;
  }
  return buffer;
}
bool Pipe::write(std::vector<char> const &bytes) const {

  int num, byteCount = bytes.size();
  std::vector<char> buffer(sizeof(int) + byteCount);
  memcpy(buffer.data(), &byteCount, sizeof(byteCount));
  memcpy(buffer.data() + sizeof(byteCount), bytes.data(), byteCount);
  byteCount = buffer.size();

  while (byteCount > 0) {
    if ((num = pipe_write(m_fifoHandle, buffer.data() + (buffer.size() - byteCount), byteCount)) < 0) {
      return false;
    }
    byteCount -= num;
  }
  return true;
}
void Pipe::close() {
  if (m_fifoHandle >= 0) {
    pipe_close(m_fifoHandle);
    m_fifoHandle = -1;
  }
}
bool Pipe::valid() const {
  return m_fifoHandle != -1;
}
Pipe::operator bool() const {
  return valid();
}
std::string Pipe::path() const {
  return m_path;
}

}