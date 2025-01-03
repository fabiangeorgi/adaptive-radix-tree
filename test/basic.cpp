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

// NODE TESTS
TEST(Node4, InsertValue) {
    uint64_t test = 156;
    Key key{test};

    auto node = Node4();


    Value value = 1;
    auto child = reinterpret_cast<Node *>(value);
    node.addChildren(key[0], child);

    ASSERT_EQ(node.numberOfChildren, 1);
    ASSERT_EQ(reinterpret_cast<Value>(node.children[0]), 1);
}

TEST(Node4, InsertNode) {
    uint64_t test = 156;
    Key key{test};

    auto node = Node4();
    auto parentNode = Node4();

    Value value = 1;
    auto child = reinterpret_cast<Node *>(value);
    node.addChildren(key[1], child);
    parentNode.addChildren(key[0], dynamic_cast<Node *>(&node));

    ASSERT_EQ(node.numberOfChildren, 1);
    ASSERT_EQ(node.children[0], child);
    ASSERT_EQ(node.numberOfChildren, 1);
    ASSERT_EQ(reinterpret_cast<Value>(node.children[0]), 1);
}

//GROW TESTS
TEST(Node, grow) {
    auto node4 = Node4();

    for (uint8_t i = 0; i < 4; i++) {
        Value value = i;
        auto child = reinterpret_cast<Node *>(value);
        node4.addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node4.children[i]), i);
    }
    ASSERT_EQ(node4.numberOfChildren, 4);
    ASSERT_TRUE(node4.isFull());

    auto node16 = node4.grow();
    for (uint8_t i = 4; i < 16; i++) {
        Value value = i;
        auto child = reinterpret_cast<Node *>(value);

        node16->addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node16->children[i]), i);
    }
    ASSERT_EQ(node16->numberOfChildren, 16);
    ASSERT_TRUE(node16->isFull());

    auto node48 = node16->grow();
    for (uint8_t i = 16; i < 48; i++) {
        Value value = i;
        auto child = reinterpret_cast<Node *>(value);

        node48->addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node48->children[node48->keys[i]]), i);
    }
    ASSERT_EQ(node48->numberOfChildren, 48);

    auto node256 = node48->grow();
    ASSERT_EQ(node256->numberOfChildren, 48);
    for (uint8_t i = 0; i < 48; i++) {
        ASSERT_EQ(reinterpret_cast<Value>(node256->children[i]), i);
    }

}

TEST(Node, growLargerValuesAboveUnused) {
    const int MULTIPLICATION_FACTOR = 1000;

    auto node4 = Node4();

    for (uint8_t i = 0; i < 4; i++) {
        Value value = i * MULTIPLICATION_FACTOR;
        auto child = reinterpret_cast<Node *>(value);
        node4.addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node4.children[i]), i * MULTIPLICATION_FACTOR);
    }
    ASSERT_EQ(node4.numberOfChildren, 4);
    ASSERT_TRUE(node4.isFull());


    auto node16 = node4.grow();
    for (uint8_t i = 4; i < 16; i++) {
        Value value = i * MULTIPLICATION_FACTOR;
        auto child = reinterpret_cast<Node *>(value);

        node16->addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node16->children[i]), i * MULTIPLICATION_FACTOR);
    }
    ASSERT_EQ(node16->numberOfChildren, 16);
    ASSERT_TRUE(node16->isFull());

    auto node48 = node16->grow();
    for (uint8_t i = 16; i < 48; i++) {
        Value value = i * MULTIPLICATION_FACTOR;
        auto child = reinterpret_cast<Node *>(value);

        node48->addChildren(i, child);
        ASSERT_EQ(reinterpret_cast<Value>(node48->children[node48->keys[i]]), i * MULTIPLICATION_FACTOR);
    }
    ASSERT_EQ(node48->numberOfChildren, 48);

    auto node256 = node48->grow();
    ASSERT_EQ(node256->numberOfChildren, 48);
    for (uint8_t i = 0; i < 48; i++) {
        ASSERT_EQ(reinterpret_cast<Value>(node256->children[i]), i * MULTIPLICATION_FACTOR);
    }

}

// TREE TESTS
TEST(ART, InsertKey) {
    ART index{};
    uint64_t test = 156;
    Key key{test};

    EXPECT_TRUE(index.insert(key, 1));
}

TEST(ART, InsertKey2) {
    ART index{};
    uint64_t test = 156;
    Key key{test};

    EXPECT_TRUE(index.insert(key, 1));

    uint64_t test2 = 157;
    Key key2{test2};

    EXPECT_TRUE(index.insert(key2, 2));
}

// I get a segmentation fault error -> I don't know why this test does not fail then
TEST(ART, GrowingNodesToNode16) {
    int numberOfInputs = 5;
    ART index{};
    std::array<uint64_t, 5> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, GrowingNodesToNode48) {
    int numberOfInputs = 17;
    ART index{};
    std::array<uint64_t, 17> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, GrowingNodesToNode256) {
    int numberOfInputs = 200;
    ART index{};
    std::array<uint64_t, 200> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, ManyInsertions1) {
    int numberOfInputs = 500;
    ART index{};
    std::array<uint64_t, 500> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, ManyInsertionsTest) {
    int numberOfInputs = 256;
    ART index{};
    std::array<uint64_t, 256> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, ManyInsertions256) {
    int numberOfInputs = 255;
    ART index{};
    std::array<uint64_t, 255> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }
    Key key{256};
    ASSERT_TRUE(index.insert(key, 255));

    Key lookup_key{256};
    EXPECT_EQ(index.lookup(lookup_key), 255);
}

TEST(ART, ManyInsertions2) {
    int numberOfInputs = 1000;
    ART index{};
    std::array<uint64_t, 1000> keys{};

    for (uint64_t i = 0; i < numberOfInputs; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= numberOfInputs; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, ManyInsertions3) {
    ART index{};
    std::array<uint64_t, 10000> keys{};

    for (uint64_t i = 0; i < 10000; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }
}

TEST(ART, ManyInsertions4) {
    ART index{};
    std::array<uint64_t, 100000> keys{};

    for (uint64_t i = 0; i < 100000; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }
}

TEST(ART, EdgeCasesTest) {
    ART index{};

    Key maxKey{18446744073709551615};
    Key minKey{1};
    Key specialPatterns{9187201950435737471};

    ASSERT_TRUE(index.insert(maxKey, 1));
    ASSERT_TRUE(index.insert(minKey, 2));
    ASSERT_TRUE(index.insert(specialPatterns, 3));

    EXPECT_EQ(index.lookup(maxKey), 1);
    EXPECT_EQ(index.lookup(minKey), 2);
    EXPECT_EQ(index.lookup(specialPatterns), 3);
}

TEST(ART, SpecialPatterns) {
    ART index{};

    Key specialPatternOne{9187201950435737471};
    Key specialPatternTwo{9187201948305031039};
    Key specialPatternThree{9151454626262777727};
    Key specialPatternFour{140183445929855};
    Key specialPatternFive{9187201950435737344};

    ASSERT_TRUE(index.insert(specialPatternOne, 1));
    ASSERT_TRUE(index.insert(specialPatternTwo, 2));
    ASSERT_TRUE(index.insert(specialPatternThree, 3));
//    ASSERT_TRUE(index.insert(specialPatternFour, 4));
//    ASSERT_TRUE(index.insert(specialPatternFive, 5));

    EXPECT_EQ(index.lookup(specialPatternOne), 1);
    EXPECT_EQ(index.lookup(specialPatternTwo), 2);
    EXPECT_EQ(index.lookup(specialPatternThree), 3);
//    EXPECT_EQ(index.lookup(specialPatternFour), 4);
//    EXPECT_EQ(index.lookup(specialPatternFive), 5);
}

// 01111111 01111111 01111111 01111111 01111111 01111111 01111111 01111111 = 9187201950435737471
// 01111111 01111111 01111111 01111111 00000000 01111111 01111111 01111111 = 9187201948305031039
// 01111111 00000000 01111111 01111111 00000000 01111111 01111111 01111111 = 9151454626262777727
// 00000000 00000000 01111111 01111111 00000000 01111111 01111111 01111111 = 140183445929855
// 01111111 01111111 01111111 01111111 01111111 01111111 01111111 00000000 = 9187201950435737344

TEST(ART, ManyInsertionsReverse) {
    ART index{};
    std::array<uint64_t, 100000> keys{};

    for (uint64_t i = 100000; i > 0; i--) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= 100001; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
}

TEST(ART, StringKeysPatterns) {
    const uint8_t key_len = 5; // ignore \0 byte
    std::array<const char *, 5> keys = {
            "fooo0", "foo0o", "fo0oo", "f0ooo", "0fooo"
    };

    ART index{};
    for (size_t i = 0; i < 5; ++i) {
        ASSERT_TRUE(index.insert(Key{keys[i], key_len}, i + 1));
    }

    EXPECT_EQ(index.lookup(Key{"fooo0", key_len}), 1);
    EXPECT_EQ(index.lookup(Key{"foo0o", key_len}), 2);
    EXPECT_EQ(index.lookup(Key{"fo0oo", key_len}), 3);
    EXPECT_EQ(index.lookup(Key{"f0ooo", key_len}), 4);
    EXPECT_EQ(index.lookup(Key{"0fooo", key_len}), 5);
}


TEST(ART, ManyInsertions5) {
    ART index{};
    std::array<uint64_t, 1000000> keys{};

    for (uint64_t i = 0; i < 1000000; i++) {
        keys[i] = i + 1;
        Key key{keys[i]};
        ASSERT_TRUE(index.insert(key, i));
    }

    for (uint64_t i = 1; i <= 1000000; i++) {
        Key lookup_key{i};
        EXPECT_EQ(index.lookup(lookup_key), i - 1);
    }
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

TEST(ART, MultiInsertLookup) {
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
    std::array<const char *, 10> keys = {
            "foo0", "foo1", "fo2o", "foo3", "f4o0", "5foo", "foo6", "f7oo", "fo8o", "foo9"
    };

    ART index{};
    for (size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(index.insert(Key{keys[i], key_len}, i + 1));
    }

    EXPECT_EQ(index.lookup(Key{"foo3", key_len}), 4);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
