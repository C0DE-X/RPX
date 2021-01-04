#include <rpx/RemoteManager.h>
#include <rpx/RemoteObject.h>
#include <thread>

namespace rpx {

struct RemotePack {
  std::string objID;
  std::string funcID;
  unsigned bufferID {0};
  bool response {false};
  bytearray args;

  RemotePack() = default;
  RemotePack(std::string oID, std::string fID, unsigned bufID,
             bool res, bytearray a)
      : objID(std::move(oID)), funcID(std::move(fID)), bufferID(bufID), response(res), args(std::move(a)) {}
  ~RemotePack() = default;
  MSGPACK_DEFINE(objID, funcID, bufferID, response, args)
};

RemoteManager &RemoteManager::instance() {
  static RemoteManager rm;
  return rm;
}

void RemoteManager::addCommunication(ICommunication *com) {

  if (com) {
    RemoteManager &manager = instance();
    com->setOnRecv([&manager = manager](bytearray const &buffer) {
      manager.recvRemote(buffer);
    });
    manager.m_connections.push_back(com);
  }
}

void RemoteManager::addRemote(RemoteObject *object) {
  std::lock_guard<std::recursive_mutex> guard(m_muxObj);
  if (m_objects.find(object->id()) != m_objects.end())
    std::cout << "Warning: Registered object " << object->id()
              << " gets overwritten" << std::endl;
  m_objects[object->id()] = object;
}

void RemoteManager::removeRemote(RemoteObject *object) {
  std::lock_guard<std::recursive_mutex> guard(m_muxObj);
  auto find = m_objects.find(object->id());
  while (find != m_objects.end() && find->first == object->id()) {
    if (find->second == object) {
      m_objects.erase(find);
      break;
    }
    ++find;
  }
}

bool RemoteManager::callRemote(RemoteObject *object, std::string const &funcID,
                               bytearray const &args) {
  bytearray dummy;
  return callRemote(object, funcID, args, dummy);
}

bool RemoteManager::callRemote(RemoteObject *object, const std::string &funcID,
                               const bytearray &args, bytearray &ret) {

  RemotePack rb(object->id(), funcID, uID(), false, args);
  Result result(this, rb.bufferID);

  auto buffer = Utils::pack(rb);

  for (auto &com : m_connections)
    com->send(buffer);

  result.wait(DEFAULT_TIMEOUT);

  if (result.isValid())
    ret = result.getValue();

  return result.isValid();
}

void RemoteManager::recvRemote(bytearray const &buffer) {
  auto rb = Utils::unpack<RemotePack>(buffer);
  if (rb.response) {
    std::lock_guard<std::mutex> guard(m_muxResp);

    if (auto result = m_results.find(rb.bufferID); result != m_results.end())
      result->second->setValue(rb.args);

  } else {
    bytearray ret;
    m_muxObj.lock();
    auto obj = m_objects.find(rb.objID);
    if (obj != m_objects.end())
      obj->second->recvRemote(rb.funcID, rb.args, ret);
    m_muxObj.unlock();

    rb.response = true;
    rb.args = ret;
    auto sbuffer = Utils::pack(rb);

    for (auto &com : m_connections)
      com->send(sbuffer);
  }
}

unsigned RemoteManager::uID() {
  static unsigned id = 0;
  return id++;
}

RemoteManager::Result::Result(RemoteManager *manager, unsigned id)
    : m_id(id), m_manager(manager) {
  std::lock_guard<std::mutex> guard(m_manager->m_muxResp);
  m_manager->m_results.insert({id, this});
}

RemoteManager::Result::~Result() {
  std::lock_guard<std::mutex> guard(m_manager->m_muxResp);
  if (auto find = m_manager->m_results.find(m_id);
      find != m_manager->m_results.end())
    m_manager->m_results.erase(find);
}

bool RemoteManager::Result::isValid() const { return m_valid; }

void RemoteManager::Result::setValue(const bytearray &value) {
  m_value = value;
  m_valid = true;
}

void RemoteManager::Result::wait(int timeout_ms) const {
  using stopwatch = std::chrono::steady_clock;

  auto start = stopwatch::now();
  while (!m_valid && (std::chrono::duration_cast<std::chrono::milliseconds>(
                          stopwatch::now() - start)
                          .count() <= timeout_ms)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::this_thread::yield();
  }
}

bytearray RemoteManager::Result::getValue() const { return m_value; }

} // namespace rpx
