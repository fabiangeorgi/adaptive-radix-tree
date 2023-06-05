#include "gtest/gtest.h"

#include "art.hpp"

#include <iostream>
#include <array>
#include <random>
#include <algorithm>

// This is a small helper test to help you understand the layout of our Key struct.
TEST(ART, BasicKeys) {
  uint64_t test = 156;
  test <<= 8; // shift 156 by one byte
  test |= 255; // add 255 as the last byte
  Key int_key{test};

  std::cout << "test key length: " << static_cast<uint32_t>(int_key.key_len) << '\n';
  for (size_t i = 0; i < int_key.key_len; i++) {
    std::cout << "Byte #" << i << " = " << static_cast<uint16_t>(int_key[i]) << '\n';
  }
  std::cout << "\nOr with byte util function:\n";
  std::cout << "test key: " << int_key.as_bytes();
  std::cout << "\n\n";

  Key str_key{"foobar", 6};
  std::cout << "foobar key length: " << static_cast<uint32_t>(str_key.key_len) << '\n';
  for (size_t i = 0; i < str_key.key_len; i++) {
    std::cout << "Byte #" << i << " = " << static_cast<char>(str_key[i]) << '\n';
  }
  std::cout << "\nOr with byte util function: " << str_key.as_bytes();
  std::cout << "\nOr with string util function: " << str_key.as_string();
}

TEST(ART, InsertKey) {
  ART index{};
  uint64_t test = 156;
  Key key{test};

  EXPECT_TRUE(index.insert(key, 1));
}

TEST(ART, InsertAndLookup) {
  ART index{};
  std::array<uint64_t, 10> keys{};

  for (uint64_t i = 0; i < 10; i++) {
    keys[i] = i + 1;
    Key key{keys[i]};
    ASSERT_TRUE(index.insert(key, i));
  }

  Key lookup_key{8};
  EXPECT_EQ(index.lookup(lookup_key), 7);
}

TEST(ART, MultiInsertLoookup) {
  ART index{};
  std::array<uint64_t, 230> keys{};

  for (uint64_t i = 0; i < 230; i++) {
    keys[i] = i + 1;
    Key key{keys[i]};
    ASSERT_TRUE(index.insert(key, i));
  }

  Key lookup_key{8};
  EXPECT_EQ(index.lookup(lookup_key), 7);
}

TEST(ART, InsertLookupRandom) {
  ART index{};
  std::array<uint64_t, 230> keys{};

  for (uint64_t i = 0; i < 230; i++) {
    keys[i] = i + 1;
  }

  // Use fixed seed for deterministic test.
  std::shuffle(keys.begin(), keys.end(), std::mt19937{568745});

  for (uint64_t i = 0; i < 230; i++) {
    Key key{keys[i]};
    ASSERT_TRUE(index.insert(key, i));
  }

  Key lookup_key{keys[8]};
  EXPECT_EQ(index.lookup(lookup_key), 8);
}

TEST(ART, StringKeys) {
  const uint8_t key_len = 4; // ignore \0 byte
  std::array<const char*, 10> keys = {
      "foo0", "foo1", "fo2o", "foo3", "f4o0", "5foo", "foo6", "f7oo", "fo8o", "foo9"
  };

  ART index{};
  for (size_t i = 0; i < 10; ++i) {
    ASSERT_TRUE(index.insert(Key{keys[i], key_len}, i + 1));
  }

  EXPECT_EQ(index.lookup(Key{"foo3", key_len}), 4);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}