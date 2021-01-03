#include "rpx/PipeNode.h"
#include <cstring>
#include <queue>
#include <iostream>
#include <cmath>

namespace rpx::communication {

namespace pipenode {
enum MSGTYPE {
  CONNECT = 0,
  MSG
};

}

PipeNode::PipeNode(std::string const &path)
    : m_pipe(path), m_framebuffer(createFramebuffer(path)) {
  m_reader = new std::thread(&PipeNode::read_t, this);
}
PipeNode::~PipeNode() {
  m_pipe.close();
  m_reader->join();
  delete m_reader;
  for (auto &p : m_pipes)
    p.close();
}
void PipeNode::read_t() {
  std::map<std::string, std::vector<std::vector<char>>> msgqueue;

  while (m_pipe) {
    auto bytes = m_pipe.read();
    if (!bytes.empty()) {
      auto type = static_cast<pipenode::MSGTYPE>(bytes[0]);
      switch (type) {
      case pipenode::CONNECT: {
        acceptConnect(std::string(bytes.begin() + 1, bytes.end()));
        break;
      }
      case pipenode::MSG: {
        if (acceptMessage(bytes, msgqueue) && m_callback)
          m_callback(bytes);
        break;
      }
      }
    }
  }
}
bool PipeNode::send(rpx::bytearray const &message) {

  std::vector<char> buffer = m_framebuffer;
  size_t chunkSize = MAXSIZE - m_framebuffer.size();
  int chunkCount = ceil((message.size() * 1.0) / chunkSize);
  memcpy(buffer.data() + 1 + sizeof(int), &chunkCount, sizeof(chunkCount));
  const char *chunkBegin = message.data();

  std::lock_guard<std::mutex> guard(m_lock);
  for (int i = 0; i < chunkCount; ++i) {
    memcpy(buffer.data() + 1, &i, sizeof(i));
    size_t chunkLength = message.size() - i * chunkSize;
    if (chunkLength > chunkSize)
      chunkLength = chunkSize;
    buffer.resize(m_framebuffer.size() + chunkLength);
    memcpy(buffer.data() + m_framebuffer.size(), chunkBegin, chunkLength);
    chunkBegin += chunkLength;
    for (auto iter = m_pipes.begin(); iter != m_pipes.end();)
      iter = !iter->write(buffer) ? m_pipes.erase(iter) : ++iter;
  }
  return true;
}
void PipeNode::setOnRecv(RecvCallback const &cb) {
  m_callback = cb;
}
bool PipeNode::connectTo(const std::string &path) {
  if (!path.empty()) {
    std::vector<char> bytes(1 + m_pipe.path().length());
    bytes[0] = static_cast<char>(pipenode::CONNECT);
    memcpy(bytes.data() + 1, m_pipe.path().c_str(), m_pipe.path().length());

    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](Pipe const &p) {
                       return p.path() == path;
                     })
        == m_pipes.end()) {
      Pipe pipe(path);
      if (pipe.write(bytes)) {
        m_pipes.push_back(pipe);
        return true;
      }
    } else {
      return true;
    }
  }
  return false;
}
void PipeNode::acceptConnect(std::string const &path) {
  if (!path.empty()) {
    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](Pipe const &p) {
                       return p.path() == path;
                     })
        == m_pipes.end()) {
      m_pipes.emplace_back(path);
    }
  }
}
rpx::bytearray PipeNode::createFramebuffer(const std::string &path) {
  rpx::bytearray framebuffer;
  size_t pathLength = path.length();
  framebuffer.resize(1 + 2 * sizeof(int) + sizeof(pathLength) + pathLength);
  framebuffer[0] = static_cast<char>(pipenode::MSG);
  char *framebufferIter = framebuffer.data() + 1 + 2 * sizeof(int);
  memcpy(framebufferIter, &pathLength, sizeof(pathLength));
  framebufferIter += sizeof(pathLength);
  memcpy(framebufferIter, path.c_str(), pathLength);
  return framebuffer;
}
bool PipeNode::acceptMessage(std::vector<char> &buffer,
                             std::map<std::string, std::vector<std::vector<char>>> &msgqueue) {
  int index = 0, count = 0;
  size_t pathLength = 0;
  auto byteIter = buffer.begin() + 1;
  memcpy(&index, &*byteIter, sizeof(index));
  byteIter += sizeof(index);
  memcpy(&count, &*byteIter, sizeof(count));
  byteIter += sizeof(count);
  memcpy(&pathLength, &*byteIter, sizeof(pathLength));
  byteIter += sizeof(pathLength);
  std::string pipepath(byteIter, byteIter + pathLength);
  byteIter += pathLength;
  buffer.erase(buffer.begin(), byteIter);
  if (count == 1) {
    return true;
  } else if (count > 1) {
    std::vector<std::vector<char>> &queue = msgqueue[pipepath];
    queue.resize(count);
    queue[index] = buffer;
    if (index == count - 1) {
      buffer.clear();
      for (int i = 0; i < count; ++i)
        buffer.insert(buffer.end(), queue[i].begin(), queue[i].end());
      msgqueue.erase(pipepath);
      return true;
    }
  }
  return false;
}
}