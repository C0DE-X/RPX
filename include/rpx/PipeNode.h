#pragma once

#include <rpx/ICommunication.h>

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <list>
#include <memory>
#include <msgpack.hpp>

namespace rpx::communication {

namespace pipe
{
class Pipe;
class PipeMessageHandler;
}

class PipeNode : public ICommunication{

public:
  explicit PipeNode(std::string const &path);
  ~PipeNode();

  bool connectTo(std::string const &path);
  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  std::unique_ptr<pipe::PipeMessageHandler> m_messageHandler;
  std::unique_ptr<pipe::Pipe> m_pipe;
  std::list<pipe::Pipe*> m_pipes;
  mutable std::mutex m_lock;
  std::thread *m_reader;
  RecvCallback m_callback;

  void receive_t();
  void acceptConnect(std::string const& path);
  static rpx::bytearray acceptMessage(unsigned long index, unsigned long count, std::string const& path, rpx::bytearray const& content,
                                      std::map<std::string, std::vector<std::vector<char>>> &msgqueue);
};

}
