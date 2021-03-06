#pragma once

#include <vector>
#include <string>

namespace rpx::communication {
class Pipe {
public:
  static constexpr size_t MSG_PREFIX = sizeof(unsigned long);
  static constexpr size_t MAX_MSGSIZE = 4096;

  explicit Pipe(std::string path);
  Pipe(Pipe const &other);
  Pipe(Pipe &&other) noexcept;
  virtual ~Pipe();

  Pipe &operator=(Pipe const &other);
  Pipe &operator=(Pipe &&other) noexcept;
  explicit operator bool() const;

  [[nodiscard]] bool valid() const;
  [[nodiscard]] std::string path() const;
  [[nodiscard]] std::vector<char> read(size_t count) const;
  [[nodiscard]] bool write(std::vector<char> const &bytes) const;
  void close();

private:
  std::string m_path;
  int m_fifoHandle;
};
}

