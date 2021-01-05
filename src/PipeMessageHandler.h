#pragma once
#include <vector>
#include <string>
#include <msgpack.hpp>
#include <rpx/Globals.h>
#include "Pipe.h"

namespace rpx::communication::pipe {
class PipeMessageHandler {
public:
  static constexpr int BUFFER_MAX = 4096;
  enum MSGTYPE {
    INVALID = 0,
    CONNECT,
    MSG
  };
  struct MsgBuffer
  {
    int type{INVALID};
    unsigned long index{0};
    unsigned long count{0};
    std::string path;
    std::vector<char> content;
    MSGPACK_DEFINE(type, index, count, path, content)
  };
  explicit PipeMessageHandler(std::string const& path);
  ~PipeMessageHandler() = default;

  static MsgBuffer read(Pipe& pipe);
  bool write(Pipe& pipe, MSGTYPE type, rpx::bytearray const& bytes = rpx::bytearray()) const;

private:
  unsigned long const m_frameSize;
  std::string const m_path;
};
}


