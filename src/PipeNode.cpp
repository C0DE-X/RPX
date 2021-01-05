#include "rpx/PipeNode.h"
#include "Pipe.h"
#include "rpx/Utils.h"
#include <queue>
#include <numeric>
#include "PipeMessageHandler.h"

namespace rpx::communication {

PipeNode::PipeNode(std::string const &path)
    : m_messageHandler(new pipe::PipeMessageHandler(path)), m_pipe(new pipe::Pipe(path)){
  m_reader = new std::thread(&PipeNode::receive_t, this);
}
PipeNode::~PipeNode() {
  m_pipe->close();
  m_reader->join();
  for (auto p : m_pipes) {
    p->close();
    delete p;
  }
  delete m_reader;
}
void PipeNode::receive_t() {
  std::map<std::string, std::vector<std::vector<char>>> msgqueue;
  while (m_pipe->valid()) {
    pipe::PipeMessageHandler::MsgBuffer buffer = pipe::PipeMessageHandler::read(*m_pipe);
      switch (buffer.type) {
      case pipe::PipeMessageHandler::CONNECT: {
        acceptConnect(buffer.path);
        break;
      }
      case pipe::PipeMessageHandler::MSG: {
        rpx::bytearray bytes = acceptMessage(buffer.index, buffer.count, buffer.path, buffer.content, msgqueue);
        if (!bytes.empty() && m_callback)
          m_callback(bytes);
        break;
      }
      default:
        break;
      }
  }
}
bool PipeNode::send(rpx::bytearray const &message) {

  for (auto iter = m_pipes.begin(); iter != m_pipes.end();)
  {
    if(!m_messageHandler->write(**iter, pipe::PipeMessageHandler::MSG, message)){
      delete *iter;
      iter = m_pipes.erase(iter);
    }else
      ++iter;
  }
  return true;
}
void PipeNode::setOnRecv(RecvCallback const &cb) {
  m_callback = cb;
}
bool PipeNode::connectTo(const std::string &path) {
  if (!path.empty()) {
    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](pipe::Pipe* p) {
                       return p->path() == path;
                     })
        == m_pipes.end()) {
      pipe::Pipe pipe(path);
      if (m_messageHandler->write(pipe, pipe::PipeMessageHandler::CONNECT)) {
        m_pipes.push_back(new pipe::Pipe(pipe));
        return true;
      }
    } else {
      return true;
    }
  }
  return false;
}

void PipeNode::acceptConnect(std::string const& path) {
  if (!path.empty()) {
    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](pipe::Pipe *p) {
                       return p->path() == path;
                     })
        == m_pipes.end()) {
      m_pipes.push_back(new pipe::Pipe(path));
    }
  }
}
rpx::bytearray PipeNode::acceptMessage(unsigned long index, unsigned long count, std::string const& path,
                                       rpx::bytearray const& content,
                                       std::map<std::string, std::vector<std::vector<char>>> &msgqueue) {
  if (count == 1) {
    return content;
  } else if (count > 1) {
    std::vector<std::vector<char>> &queue = msgqueue[path];
    queue.resize(count);
    queue[index] = content;
    if (index == count - 1)
      return std::accumulate(queue.begin(), queue.end(), rpx::bytearray(),
                             [](rpx::bytearray &a, rpx::bytearray const& b)
                             { a.insert(a.end(), b.begin(), b.end()); return a; });
  }
  return rpx::bytearray();
}
}