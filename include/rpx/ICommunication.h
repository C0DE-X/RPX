#pragma once

#include <functional>
#include <rpx/Globals.h>

namespace rpx {

class ICommunication {

public:
  using RecvCallback = std::function<void(rpx::bytearray const &)>;

  virtual bool send(rpx::bytearray const &message) = 0;
  virtual void setOnRecv(RecvCallback const &cb) = 0;
};

} // namespace rpx