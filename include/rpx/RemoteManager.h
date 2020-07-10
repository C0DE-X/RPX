#ifndef REMOTEMANAGER_H
#define REMOTEMANAGER_H

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <rpx/Client.h>
#include <rpx/Server.h>
#include <rpx/Utils.h>

namespace rpx {

class RemoteObject;

class RemoteManager {

  static constexpr int DEFAULT_TIMEOUT = 10000;

public:
  static void listen(int port);
  static bool connect(std::string const &ip, int port);

  static RemoteManager &instance();
  RemoteManager(RemoteManager const &) = delete;
  RemoteManager(RemoteManager &&) = delete;
  ~RemoteManager() = default;

  RemoteManager &operator=(RemoteManager const &) = delete;
  RemoteManager &operator=(RemoteManager &&) = delete;

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

    bool isValid() const;
    void setValue(bytearray const &value);
    void wait(int timeout_ms);
    bytearray getValue() const;

  private:
    unsigned m_id{0};
    bool m_valid{false};
    RemoteManager *m_manager;
    bytearray m_value;
  };

  RemoteManager();

  std::multimap<std::string, RemoteObject *> m_objects;
  std::map<unsigned, Result *> m_results;
  mutable std::mutex m_muxResp;
  mutable std::recursive_mutex m_muxObj;

  network::Client m_client;
  network::Server m_server;

  void recvRemote(const bytearray &buffer);
  unsigned uID() const;
};

} // namespace rpx

#endif // REMOTEMANAGER_H
