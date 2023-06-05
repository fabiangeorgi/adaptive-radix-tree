#pragma once

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

/** Our value type is always a uint64_t. */
using Value = uint64_t;

/** 0 represents a special "invalid" value. This should be returned when no entry is found. */
constexpr Value INVALID_VALUE = 0;

/**
 * This is the key class that represents keys in ART. It is a basic wrapper around an array of 8 bytes, in which you can
 * store the key data.
 *
 * Do not modify this struct.
 */
struct alignas(8) Key {
  std::array<uint8_t, 8> key{};
  uint8_t key_len = 0;

  /** Constructor for numeric 8-Byte keys */
  explicit Key(uint64_t k) { set_int(k); }

  /** Constructor for non-uint64_t keys. */
  Key(const char bytes[], uint8_t length) { set(bytes, length); }

  Key() = default;
  ~Key() = default;

  Key(const Key& key) = default;
  Key& operator=(const Key& key) = default;
  Key(Key&& key) = default;
  Key& operator=(Key&&) = default;

  void set(const char bytes[], uint8_t length) {
    assert(length <= 8);
    std::memcpy(key.data(), bytes, length);
    key_len = length;
  }

  void set_int(uint64_t k) {
    // Reverse order of bytes to have most significant byte first.
    *reinterpret_cast<uint64_t*>(key.data()) = __builtin_bswap64(k);
    key_len = 8;
  }

  bool operator==(const Key& other) const {
    return (key_len == other.key_len) && (std::memcmp(key.data(), other.key.data(), key_len) == 0);
  }

  uint8_t& operator[](uint8_t i) {
    assert(i < key_len);
    return key[i];
  }

  const uint8_t& operator[](uint8_t i) const {
    assert(i < key_len);
    return key[i];
  }

  /** Returns a the individual bytes of the key as bit stings. */
  std::string as_bytes() const {
    std::stringstream out;
    for (size_t i = 0; i < key_len; ++i) {
      out << std::bitset<8>(key[i]) << ' ';
    }
    return out.str();
  }

  /** Returns a the individual bytes of the key as printable characters. */
  std::string as_string() const {
    std::stringstream out;
    for (size_t i = 0; i < key_len; ++i) {
      out << key[i] << ' ';
    }
    return out.str();
  }
};
