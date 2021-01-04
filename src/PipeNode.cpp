#include "rpx/PipeNode.h"
#include "Pipe.h"
#include "rpx/Utils.h"
#include <cstring>
#include <queue>
#include <cmath>

namespace rpx::communication {

namespace pipenode {
enum MSGTYPE {
  CONNECT = 0,
  MSG
};
}

PipeNode::PipeNode(std::string const &path)
    : m_pipe(new Pipe(path)), m_msghead(createMsgHead(path)) {
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
  delete m_pipe;
}
void PipeNode::receive_t() {
  std::map<std::string, std::vector<std::vector<char>>> msgqueue;
  while (m_pipe->valid()) {
    auto bytes = read();
    if (!bytes.empty()) {
      auto type = static_cast<pipenode::MSGTYPE>(bytes[0]);
      switch (type) {
      case pipenode::CONNECT: {
        acceptConnect(bytes);
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
  return write(static_cast<rpx::byte>(pipenode::MSG), message);
}
void PipeNode::setOnRecv(RecvCallback const &cb) {
  m_callback = cb;
}
bool PipeNode::connectTo(const std::string &path) {
  if (!path.empty()) {
    std::vector<char> bytes(BYTE_SIZE + LONG_SIZE + m_pipe->path().length());
    auto sizeBuffer = Utils::fromSize(bytes.size());
    char* bytesIter = bytes.data();
    memcpy(bytesIter, sizeBuffer.data(), sizeBuffer.size());
    bytesIter += sizeBuffer.size();
    *bytesIter = static_cast<char>(pipenode::CONNECT);
    memcpy(++bytesIter, m_pipe->path().c_str(), m_pipe->path().length());

    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](Pipe* p) {
                       return p->path() == path;
                     })
        == m_pipes.end()) {
      Pipe pipe(path);
      if (pipe.write(bytes)) {
        m_pipes.push_back(new Pipe(pipe));
        return true;
      }
    } else {
      return true;
    }
  }
  return false;
}
rpx::bytearray PipeNode::createMsgHead(const std::string &path) {
  rpx::bytearray buffer;
  unsigned long pathLength = path.length();
  buffer.resize(sizeof(std::byte) + 4 * LONG_SIZE + pathLength);
  char *iter = buffer.data() + BYTE_SIZE + 3 * LONG_SIZE;
  Utils::fromArithmetic(pathLength, iter);
  iter += sizeof(pathLength);
  memcpy(iter, path.c_str(), pathLength);
  return buffer;
}
rpx::bytearray PipeNode::read() {
  rpx::bytearray buffer;
  while (m_pipe->valid()) {
    buffer = m_pipe->read(LONG_SIZE);
    if (buffer.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
    }
    unsigned long bytes = Utils::toSize(buffer) - LONG_SIZE;
    buffer.resize(bytes);
    buffer = m_pipe->read(bytes);
    break;
  }
  return buffer;
}
bool PipeNode::write(rpx::byte type, const bytearray &bytes) {
  std::vector<char> buffer = m_msghead;
  size_t chunkSize = Pipe::MAX_MSGSIZE - m_msghead.size();
  unsigned long chunkCount = ceil((bytes.size() * 1.0) / chunkSize);
  const char *chunkBegin = bytes.data();
  buffer[LONG_SIZE] = type;
  Utils::fromArithmetic(chunkCount, buffer.data() + BYTE_SIZE + 2 * LONG_SIZE);

  std::lock_guard<std::mutex> guard(m_lock);
  for (unsigned long i = 0; i < chunkCount; ++i) {
    size_t chunkLength = bytes.size() - i * chunkSize;
    if (chunkLength > chunkSize)
      chunkLength = chunkSize;
    buffer.resize(m_msghead.size() + chunkLength);
    Utils::fromArithmetic(m_msghead.size() + chunkLength, buffer.data());
    Utils::fromArithmetic(i, buffer.data() + BYTE_SIZE + LONG_SIZE);
    memcpy(buffer.data() + m_msghead.size(), chunkBegin, chunkLength);
    chunkBegin += chunkLength;
    for (auto iter = m_pipes.begin(); iter != m_pipes.end();)
    {
      if(!(*iter)->write(buffer)){
        delete *iter;
        iter = m_pipes.erase(iter);
      }else
        ++iter;
    }
  }
  return true;
}
void PipeNode::acceptConnect(rpx::bytearray const& buffer) {
  std::string path(buffer.begin() + 1, buffer.end());
  if (!path.empty()) {
    std::lock_guard<std::mutex> guard(m_lock);
    if (std::find_if(m_pipes.cbegin(),
                     m_pipes.cend(),
                     [&path](Pipe *p) {
                       return p->path() == path;
                     })
        == m_pipes.end()) {
      m_pipes.push_back(new Pipe(path));
    }
  }
}
bool PipeNode::acceptMessage(std::vector<char> &buffer,
                             std::map<std::string, std::vector<std::vector<char>>> &msgqueue) {
  auto byteIter = buffer.begin() + BYTE_SIZE;
  auto index = Utils::toArithmetic<unsigned long>(&*byteIter);
  byteIter += LONG_SIZE;
  auto count = Utils::toArithmetic<unsigned long>(&*byteIter);
  byteIter += LONG_SIZE;
  auto pathLength = Utils::toArithmetic<size_t>(&*byteIter);
  byteIter += LONG_SIZE;
  std::string pipepath(byteIter, byteIter + pathLength);
  buffer.erase(buffer.begin(), byteIter + pathLength);
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