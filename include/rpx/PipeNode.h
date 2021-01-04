#pragma once

#include <rpx/ICommunication.h>

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <list>

namespace rpx::communication {

class Pipe;

class PipeNode : public ICommunication{

public:
  explicit PipeNode(std::string const &path);
  ~PipeNode();

  bool connectTo(std::string const &path);
  bool send(rpx::bytearray const &message) override;
  void setOnRecv(RecvCallback const &cb) override;

private:
  Pipe* m_pipe;
  std::list<Pipe*> m_pipes;
  mutable std::mutex m_lock;
  std::thread *m_reader;
  RecvCallback m_callback;
  rpx::bytearray const m_msghead;

  void receive_t();
  void acceptConnect(rpx::bytearray const& buffer);
  rpx::bytearray read();
  bool write(rpx::byte type, rpx::bytearray const& bytes);

  static rpx::bytearray createMsgHead(std::string const &path);
  static bool acceptMessage(std::vector<char> &buffer,
                            std::map<std::string, std::vector<std::vector<char>>> &msgqueue);
};

}

