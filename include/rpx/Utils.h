#pragma once

#include <algorithm>
#include <type_traits>
#include <msgpack.hpp>
#include <array>
#include "Globals.h"

namespace rpx {

static constexpr bool isLittleEndian() {
  const short number(0x1);
  return (&number)[0];
}

struct Utils {

  template <typename T = std::array<char, 2>>
  static auto fromSize(unsigned long const& size) -> std::enable_if_t<!isLittleEndian(), T> {
    std::array<char, 8> ret{};
    memcpy(ret.data(), &size, ret.size());
    return ret;
  }

  template <typename T = unsigned long>
  static auto toSize(std::array<char,8> const& buffer) -> std::enable_if_t<!isLittleEndian(), T> {    
    unsigned long ret;
    memcpy(&ret, buffer.data(), 8);
    return ret;
  }

  template <typename T = std::array<char, 8>>
  static auto fromSize(unsigned short const& size) -> std::enable_if_t<isLittleEndian(), T> {    
    std::array<char,8> ret{};
    memcpy(ret.data(), &size, 8);
    std::reverse(ret.begin(), ret.end());
    return ret;
  }

  template <typename T = unsigned short>
  static auto toSize(std::array<char,8> const& buffer) -> std::enable_if_t<isLittleEndian(), T> {    
    unsigned long ret;
    auto tmp = buffer;
    std::reverse(tmp.begin(), tmp.end());
    memcpy(&ret, tmp.data(), 8);
    return ret;
  }

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
};

} // namespace rpx