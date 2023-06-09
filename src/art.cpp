#include "art.hpp"

#include <iostream>
#include <sstream>
#include "immintrin.h"

// TODO: implement if needed
ART::ART() = default;

// TODO: implement if needed
ART::~ART() = default;

Value ART::lookup(const Key &key) {
    // TODO: implement lookup
    return INVALID_VALUE;
}

bool ART::insert(const Key &key, Value value) {
    return recursiveInsert(root, key, value, 0);
}

bool ART::recursiveInsert(Node *node, const Key &key, Value value, uint16_t depth) {
    // TODO probably will just implement the whole thing with lazy expansion because then i'll can stick to the paper
    if (node == nullptr) { // empty tree
        // add new Node 4 and set as root
        auto node4 = new Node4(true);
        node4->setChildren(key[depth], reinterpret_cast<Node *>(value));
        root = node4;
        return true;
    }
    // TODO: no lazy expansion till now
    auto nextNode = node->getChildren(key[depth]);
    if (nextNode != nullptr) {
        return recursiveInsert(nextNode, key, value, depth + 1);
    } else {
        if (node->isFull()) {
            // grow()
        }
        // TODO does not work
        auto node4 = new Node4(true);
        node4->setChildren(key[depth], reinterpret_cast<Node *>(value));
        node->setChildren(key[depth - 1], node4);
    }


    return false;
}

// NODE 4
Node *Node4::getChildren(uint8_t partOfKey) {
    for (int i = 0; i < this->keys.size(); i++) {
        if (this->keys[i] == partOfKey) {
            return this->children[i];
        }
    }
    return nullptr;
}

void Node4::setChildren(uint8_t partOfKey, Node *child) {
    this->keys[numberOfChildren] = partOfKey;
    this->children[numberOfChildren] = child;
    numberOfChildren++;
}

bool Node4::isFull() {
    return this->children.size() == 4;
}

Node16 *Node4::grow() {
    auto *node16 = new Node16(this->isLeafNode);

    node16->numberOfChildren = this->numberOfChildren;
    for (int i = 0; i < 4; i++) {
        node16->keys[i] = this->keys[i];
        node16->children[i] = this->children[i];
    }

    return node16;
}

// NODE 16
Node *Node16::getChildren(uint8_t partOfKey) {
    auto keyToSearchRegister = _mm_set1_epi8(partOfKey);
    // TODO don't know if there is a better way
    auto keysInNodeRegister = _mm_set_epi8(
            keys[15], keys[14], keys[13], keys[12],
            keys[11], keys[10], keys[9], keys[8],
            keys[7], keys[6], keys[5], keys[4],
            keys[3], keys[2], keys[1], keys[0]
    );
    auto cmp = _mm_cmpeq_epi8(keyToSearchRegister, keysInNodeRegister);
    auto mask = (1 << numberOfChildren) - 1;
    auto bitfield = _mm_movemask_epi8(cmp) & mask;

    if (bitfield) {
        return this->children[__builtin_ctz(bitfield)];
    }

    return nullptr;
}

void Node16::setChildren(uint8_t partOfKey, Node *child) {
    this->keys[numberOfChildren] = partOfKey;
    this->children[numberOfChildren] = child;
    numberOfChildren++;
}

bool Node16::isFull() {
    return this->numberOfChildren == 16;
}

Node48 *Node16::grow() {
    auto *node48 = new Node48(this->isLeafNode);

    node48->numberOfChildren = this->numberOfChildren;
    for (uint8_t i = 0; i < 16; i++) {
        // we have to use offsets in node48
        // save index as value at position key
        node48->keys[this->keys[i]] = i;
        // at index save children
        node48->children[i] = this->children[i];
    }

    return node48;
}

// NODE 48
Node *Node48::getChildren(uint8_t partOfKey) {
    auto index = this->keys[partOfKey];
    // index can only be between 0 and 47 -> so if different value -> it is an error
    // might also be suitable to fill they keys before up and then just check for the ERROR_VALUE instead of this random "48"
    if (index != UNUSED_OFFSET_VALUE) {
        return this->children[index];
    }
    return nullptr;
}

void Node48::setChildren(uint8_t partOfKey, Node *child) {
    this->children[numberOfChildren] = child;
    this->keys[partOfKey] = numberOfChildren;
    numberOfChildren++;
}

bool Node48::isFull() {
    return this->numberOfChildren == 48;
}

Node256 *Node48::grow() {
    auto node256 = new Node256(this->isLeafNode);

    node256->numberOfChildren = (uint16_t) this->numberOfChildren;
    for (uint16_t i = 0; i < 256; i++) {
        auto index = this->keys[i];
        if (index != UNUSED_OFFSET_VALUE) {
            node256->children[i] = this->children[index];
        }
    }

    return node256;
}

// NODE 256
Node *Node256::getChildren(uint8_t partOfKey) {
    return this->children[partOfKey];
}

void Node256::setChildren(uint8_t partOfKey, Node *child) {
    this->children[partOfKey] = child;
}

bool Node256::isFull() {
    return numberOfChildren == 256;
}
