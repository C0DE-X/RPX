#ifndef UTILS_H
#define UTILS_H

#include "Globals.h"
#include <msgpack.hpp>

namespace rpx {

struct Utils {

  template <typename ARG> static bytearray pack(ARG const &arg) {
    std::stringstream buffer;
    msgpack::pack(buffer, arg);
    return std::vector<char>((std::istreambuf_iterator<char>(buffer)),
                             std::istreambuf_iterator<char>());
  }

  template <typename ARG> static ARG unpack(bytearray const &buffer) {
    ARG arg;
    msgpack::object_handle handle =
        msgpack::unpack(buffer.data(), buffer.size());
    msgpack::object deserialized = handle.get();
    deserialized.convert(arg);
    return arg;
  }

  template <typename T, typename... ARGS>
  static std::tuple<T, ARGS...> push_front(std::tuple<ARGS...> const &t,
                                           T const &v) {
    return std::tuple_cat(std::make_tuple(v), t);
  }

  template <typename T, typename... ARGS>
  static std::tuple<T, ARGS...> push_back(std::tuple<ARGS...> const &t,
                                          T const &v) {
    return std::tuple_cat(t, std::make_tuple(v));
  }
};

} // namespace rpx

#endif // UTILS_H
