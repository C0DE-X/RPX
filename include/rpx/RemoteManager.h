#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <rpx/ICommunication.h>
#include <rpx/Utils.h>

namespace rpx {

class RemoteObject;

class RemoteManager {
  static constexpr int DEFAULT_TIMEOUT = 10000;

public:
  static RemoteManager &instance();
  RemoteManager(RemoteManager const &) = delete;
  RemoteManager(RemoteManager &&) = delete;
  ~RemoteManager() = default;

  RemoteManager &operator=(RemoteManager const &) = delete;
  RemoteManager &operator=(RemoteManager &&) = delete;

  static void addCommunication(ICommunication *com);

  void addRemote(RemoteObject *object);
  void removeRemote(RemoteObject *object);

  bool callRemote(RemoteObject *object, std::string const &funcID,
                  bytearray const &args);

  bool callRemote(RemoteObject *object, std::string const &funcID,
                  bytearray const &args, bytearray &ret);

private:
  class Result {
  public:
    Result(RemoteManager *manager, unsigned id);
    ~Result();

    [[nodiscard]] bool isValid() const;
    void setValue(bytearray const &value);
    void wait(int timeout_ms) const;
    [[nodiscard]] bytearray getValue() const;

  private:
    unsigned m_id{0};
    bool m_valid{false};
    RemoteManager *m_manager;
    bytearray m_value;
  };

  RemoteManager() = default;

  std::vector<ICommunication *> m_connections;
  std::map<std::string, RemoteObject *> m_objects;
  std::map<unsigned, Result *> m_results;
  mutable std::mutex m_muxResp;
  mutable std::recursive_mutex m_muxObj;

  void recvRemote(const bytearray &buffer);
  static unsigned uID() ;
};

} // namespace rpx
