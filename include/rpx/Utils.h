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

  template <typename T = std::array<char, LONG_SIZE>>
  static auto fromSize(unsigned long const& size) -> std::enable_if_t<!isLittleEndian(), T> {
    std::array<char, LONG_SIZE> ret{};
    memcpy(ret.data(), &size, ret.size());
    return ret;
  }

  template <typename T = unsigned long>
  static auto toSize(std::array<char,LONG_SIZE> const& buffer) -> std::enable_if_t<!isLittleEndian(), T> {
    unsigned long ret;
    memcpy(&ret, buffer.data(), LONG_SIZE);
    return ret;
  }
  template <typename T = unsigned long>
  static auto toSize(std::vector<char> const& buffer) -> std::enable_if_t<!isLittleEndian(), T> {
    unsigned long ret;
    if(buffer.size() >= LONG_SIZE)
    {
      std::array<char,LONG_SIZE> tmp{};
      std::copy(buffer.begin(), buffer.begin() + LONG_SIZE, tmp.begin());
      return toSize(tmp);
    }
    return ret;
  }

  template <typename T>
  static auto toArithmetic(const char *buffer) -> std::enable_if_t<!isLittleEndian() && std::is_arithmetic_v<T>, T> {
    T ret;
    memcpy(&ret, buffer, sizeof(T));
    return ret;
  }

  template <typename T = std::array<char, LONG_SIZE>>
  static auto fromSize(unsigned long const& size) -> std::enable_if_t<isLittleEndian(), T> {
    std::array<char,LONG_SIZE> ret{};
    memcpy(ret.data(), &size, LONG_SIZE);
    std::reverse(ret.begin(), ret.end());
    return ret;
  }

  template <typename T = unsigned short>
  static auto toSize(std::array<char,LONG_SIZE> const& buffer) -> std::enable_if_t<isLittleEndian(), T> {
    unsigned long ret;
    auto tmp = buffer;
    std::reverse(tmp.begin(), tmp.end());
    memcpy(&ret, tmp.data(), LONG_SIZE);
    return ret;
  }

  template <typename T = unsigned short>
  static auto toSize(std::vector<char> const& buffer) -> std::enable_if_t<isLittleEndian(), T> {
    unsigned long ret;
    if(buffer.size() >= LONG_SIZE)
    {
      std::array<char,LONG_SIZE> tmp{};
      std::copy(buffer.begin(), buffer.begin() + LONG_SIZE, tmp.begin());
      return toSize(tmp);
    }
    return ret;
  }

  template <typename T>
  static auto toArithmetic(const char *buffer) -> std::enable_if_t<isLittleEndian() && std::is_arithmetic_v<T>, T> {
    T ret;
    std::array<char, sizeof(T)> tmp;
    memcpy(tmp.data(), buffer, sizeof(T));
    std::reverse(tmp.begin(), tmp.end());
    memcpy(&ret, tmp.data(), sizeof(T));
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