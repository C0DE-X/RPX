#include <rpx/RemoteManager.h>
#include <rpx/RemoteObject.h>

#include <utility>

namespace rpx {

RemoteObject::RemoteObject(identifier id) : m_id(std::move(id)) {
  RemoteManager::instance().addRemote(this);
}

RemoteObject::~RemoteObject() { RemoteManager::instance().removeRemote(this); }

identifier RemoteObject::id() const { return m_id; }

bool RemoteObject::call(identifier const &id, bytearray const &args) {
  return RemoteManager::instance().callRemote(this, id, args);
}

bool RemoteObject::call(identifier const &id, bytearray const &args,
                        bytearray &ret) {
  auto success = RemoteManager::instance().callRemote(this, id, args, ret);
  return success;
}

} // namespace rpx
