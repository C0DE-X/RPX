#pragma once

#include <rpx/ICommunication.h>
#include "Pipe.h"

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <list>

namespace rpx::communication {

class PipeNode : public ICommunication{

  static constexpr size_t MAXSIZE = 4096;

public:
  explicit PipeNode(std::string const &path);
  ~PipeNode();

  bool connectTo(std::string const &path);
  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  Pipe m_pipe;
  std::list<Pipe> m_pipes;
  mutable std::mutex m_lock;
  std::thread *m_reader;
  RecvCallback m_callback;
  rpx::bytearray const m_framebuffer;

  void read_t();
  void acceptConnect(std::string const &path);

  static rpx::bytearray createFramebuffer(std::string const &path);
  static bool acceptMessage(std::vector<char> &buffer,
                            std::map<std::string, std::vector<std::vector<char>>> &msgqueue);
};

}

