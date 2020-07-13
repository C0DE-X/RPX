#ifndef REMOTEOBJECT_H
#define REMOTEOBJECT_H

#include <functional>
#include <iostream>
#include <msgpack.hpp>
#include <string>
#include <tuple>
#include <vector>

#include <rpx/Utils.h>

namespace rpx {

class RemoteObject {
public:
  RemoteObject(identifier const &id);
  virtual ~RemoteObject();

  template <typename... ARGS>
  void recvRemote(identifier const &id, bytearray const &buffer,
                  bytearray &ret);

  identifier id() const;

protected:
  template <class CLASS, typename RET, typename... ARGS>
  typename std::enable_if<std::is_same<RET, void>::value, void>::type
  registerRemote(identifier const &id, RET (CLASS::*func)(ARGS...));

  template <class CLASS, typename RET, typename... ARGS>
  typename std::enable_if<!std::is_same<RET, void>::value, void>::type
  registerRemote(identifier const &id, RET (CLASS::*func)(ARGS...));

  template <typename RET, typename... ARGS>
  typename std::enable_if<std::is_same<RET, void>::value, void>::type
  callRemote(identifier const &id, bool &success, ARGS... args);

  template <typename RET, typename... ARGS>
  typename std::enable_if<!std::is_same<RET, void>::value, RET>::type
  callRemote(identifier const &id, bool &success, RET defaultValue,
             ARGS... args);

private:
  std::string m_id;
  std::map<identifier, std::function<void(bytearray const &, bytearray &)>>
      m_remoteMap;

  bool call(identifier const &id, bytearray const &args);
  bool call(identifier const &id, bytearray const &args, bytearray &ret);
};

template <typename... ARGS>
void RemoteObject::recvRemote(const identifier &id, bytearray const &buffer,
                              bytearray &ret) {
  if (m_remoteMap.find(id) != m_remoteMap.end())
    m_remoteMap.at(id)(buffer, ret);
}

template <class CLASS, typename RET, typename... ARGS>
typename std::enable_if<std::is_same<RET, void>::value, void>::type
RemoteObject::registerRemote(identifier const &id,
                             RET (CLASS::*func)(ARGS...)) {
  m_remoteMap.insert(
      {id, [this, func](bytearray const &buffer, bytearray &ret) {
         std::apply(
             func, Utils::push_front(Utils::unpack<std::tuple<ARGS...>>(buffer),
                                     static_cast<CLASS *>(this)));
         ret.clear();
       }});
}

template <class CLASS, typename RET, typename... ARGS>
typename std::enable_if<!std::is_same<RET, void>::value, void>::type
RemoteObject::registerRemote(identifier const &id,
                             RET (CLASS::*func)(ARGS...)) {
  m_remoteMap.insert(
      {id, [this, func](bytearray const &buffer, bytearray &ret) {
         ret = Utils::pack(std::apply(
             func, Utils::push_front(Utils::unpack<std::tuple<ARGS...>>(buffer),
                                     static_cast<CLASS *>(this))));
       }});
}

template <typename RET, typename... ARGS>
typename std::enable_if<std::is_same<RET, void>::value, void>::type
RemoteObject::callRemote(identifier const &id, bool &success, ARGS... args) {
  success = call(id, Utils::pack(std::tuple<ARGS...>(args...)));
}

template <typename RET, typename... ARGS>
typename std::enable_if<!std::is_same<RET, void>::value, RET>::type
RemoteObject::callRemote(identifier const &id, bool &success, RET defaultValue,
                         ARGS... args) {
  bytearray ret;
  success = call(id, Utils::pack(std::tuple<ARGS...>(args...)), ret);
  return ret.empty() ? defaultValue : Utils::unpack<RET>(ret);
}

} // namespace rpx

#endif // REMOTEOBJECT_H