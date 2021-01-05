#include "PipeMessageHandler.h"
#include <rpx/Utils.h>
#include <thread>
#include <cmath>

rpx::communication::pipe::PipeMessageHandler::PipeMessageHandler(std::string const& path)
  : m_frameSize(Utils::pack(MsgBuffer({INVALID,0,0, path, {}})).size()),
  m_path(path)
{}
rpx::communication::pipe::PipeMessageHandler::MsgBuffer rpx::communication::pipe::PipeMessageHandler::read(Pipe& pipe) {
  rpx::bytearray buffer;
  MsgBuffer msgBuffer;
  while (pipe.valid()) {
    buffer = pipe.read(LONG_SIZE);
    if (buffer.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    unsigned long bytes = Utils::toSize(buffer);
    buffer.resize(bytes);
    buffer = pipe.read(bytes);
    msgBuffer = Utils::unpack<MsgBuffer>(buffer);
    break;
  }
  return msgBuffer;
}

bool rpx::communication::pipe::PipeMessageHandler::write(rpx::communication::pipe::Pipe &pipe,
                                                         rpx::communication::pipe::PipeMessageHandler::MSGTYPE type,
                                                         const rpx::bytearray &bytes) const{
  size_t chunkMaxSize = BUFFER_MAX - m_frameSize - LONG_SIZE;
  unsigned long chunkCount = bytes.empty() ? 1ul
                                           : static_cast<unsigned long>(ceil((bytes.size() * 1.0) / chunkMaxSize));
  const char *chunkBegin = bytes.data();

  MsgBuffer frame;
  frame.type = type;
  frame.path = m_path;
  frame.count = chunkCount;

  for (unsigned long i = 0; i < chunkCount; ++i) {
    size_t chunkLength = bytes.size() - i * chunkMaxSize;
    if (chunkLength > chunkMaxSize)
      chunkLength = chunkMaxSize;

    frame.index = i;
    frame.content = rpx::bytearray(chunkBegin, chunkBegin + chunkLength);
    rpx::bytearray buffer = Utils::pack(frame);
    std::array<char, 8> size = Utils::fromSize(buffer.size());
    buffer.insert(buffer.begin(), size.begin(), size.end());

    chunkBegin += chunkLength;

    if(!pipe.write(buffer))
      return false;
  }
  return true;
}
