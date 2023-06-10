#include "art.hpp"

#include <iostream>
#include <sstream>
#include "immintrin.h"

// TODO: implement if needed
ART::ART() = default;

// TODO: implement if needed
ART::~ART() = default;

Value ART::lookup(const Key &key) {
    return recursiveLookUp(root, key, 0);
}

Value ART::recursiveLookUp(Node *node, const Key &key, uint8_t depth) {
    uint8_t dummy = 1;
    if (node == nullptr) {
        return INVALID_VALUE;
    }
    if (node->isLeafNode) {
        Node *value = node->getChildren(key[key.key_len - 1], dummy);
        if (value == nullptr) {
            return INVALID_VALUE;
        }
        return reinterpret_cast<Value>(value);
    }
    Node *nextNode = node->getChildren(key[depth], dummy);
    return recursiveLookUp(nextNode, key, depth + 1);
}

bool ART::insert(const Key &key, Value value) {
    indexOfLastChildrenAccessed = 0;
    return recursiveInsert(nullptr, root, key, value, 0);
}

void ART::replaceNode(Node *newNode, Node *parentNode) {
    if (parentNode == nullptr) { // we need to set it as root
        root = newNode;
        return;
    }

    if (parentNode->type == NodeType::N4) {
        dynamic_cast<Node4 *>(parentNode)->children[indexOfLastChildrenAccessed] = newNode;
    } else if (parentNode->type == NodeType::N16) {
        dynamic_cast<Node16 *>(parentNode)->children[indexOfLastChildrenAccessed] = newNode;
    } else if (parentNode->type == NodeType::N48) {
        dynamic_cast<Node48 *>(parentNode)->children[indexOfLastChildrenAccessed] = newNode;
    } else if (parentNode->type == NodeType::N256) {
        dynamic_cast<Node256 *>(parentNode)->children[indexOfLastChildrenAccessed] = newNode;
    }
}

void ART::growAndReplaceNode(Node *parentNode, Node *&node) {
    // TODO test if this works
    if (node->type == NodeType::N4) {
        auto node4 = dynamic_cast<Node4 *>(node);
        auto node16 = node4->grow();
        node = node16;
    } else if (node->type == NodeType::N16) {
        auto node16 = dynamic_cast<Node16 *>(node);
        auto node48 = node16->grow();
        node = node48;
    } else if (node->type == NodeType::N48) {
        auto node48 = dynamic_cast<Node48 *>(node);
        auto node256 = node48->grow();
        node = node256;
    }

    replaceNode(node, parentNode);
}

// TODO might not even need parentNode -> remove
bool ART::recursiveInsert(Node *parentNode, Node *node, const Key &key, Value value, uint8_t depth) {
    // TODO: idea first implement lazy expansion and check if that is fast enough
    // later do path compression -> still don't quite understand it, so just leave it out for now
    if (node == nullptr) { // empty tree case
        // create new node and set as root => is also leaf node
        // this also means that we can store it directly compressed -> so we need to save the key
        auto node4 = new Node4(key, true);
        // we need to store the last key information -> this is identifier for this particular node
        // we still save the whole key in the node, so we can reinterpret the path
        node4->setChildren(key[key.key_len - 1], reinterpret_cast<Node *>(value));
        // set as new root
        root = node4;
        return true;
    }

    if (node->isLeafNode) {
        auto const &key2 = node->key;
        // we arbitrarily "traverse" through the tree and check that
        // (a) the depth does not become greater than the index of the last part of the key -> depth < key.key_len - 1
        // (b) the key to matches the existing path -> key2[depth] == key[depth]
        for (; depth < key.key_len - 1 && key2[depth] == key[depth]; depth += 1);
        // check if we are in the last possible layer -> we have to create a leaf node
        if (depth == key.key_len - 1) {
            // create new leaf node is full -> then grow
            // insert
            if (node->isFull()) {
                growAndReplaceNode(parentNode, node);
            }
            node->setChildren(key[depth], reinterpret_cast<Node *>(value));
        } else {
            // there is still not the full path in the tree -> so we'll just create a new parent node
            auto newInnerNode4 = new Node4(key, false);
            newInnerNode4->setChildren(node->key[depth], node);
            auto newLeafNode4 = new Node4(key, true);
            newLeafNode4->setChildren(key[key.key_len - 1], reinterpret_cast<Node *>(value));
            newInnerNode4->setChildren(key[depth], newLeafNode4);
            replaceNode(newInnerNode4, parentNode);
        }

        return true;
    }
    // traverse to next children
    auto next = node->getChildren(key[depth], indexOfLastChildrenAccessed);
    if (next == nullptr) { // there is no next -> we have to add it here
        if (node->isFull()) {
            growAndReplaceNode(parentNode, node);
        }
        auto newLeafNode = new Node4(key, true);
        newLeafNode->setChildren(key[key.key_len - 1], reinterpret_cast<Node *>(value));
        node->setChildren(key[depth], newLeafNode);
        return true;
    } else {
        // traverse further
        return recursiveInsert(node, next, key, value, depth + 1);
    }
}

// NODE 4
Node *Node4::getChildren(uint8_t partOfKey, uint8_t& indexOfLastChildrenAccessed) {
    for (uint8_t i = 0; i < this->keys.size(); i++) {
        if (this->keys[i] == partOfKey) {
            indexOfLastChildrenAccessed = i;
            return this->children[i];
        }
    }
    return nullptr;
}

void Node4::setChildren(uint8_t partOfKey, Node *child) {
    this->keys[numberOfChildren] = partOfKey;
    this->children[numberOfChildren] = child;
    this->numberOfChildren++;
}

bool Node4::isFull() {
    return this->numberOfChildren == 4;
}

Node16 *Node4::grow() {
    auto *node16 = new Node16(this->key, this->isLeafNode);

    node16->numberOfChildren = this->numberOfChildren;
    for (int i = 0; i < 4; i++) {
        node16->keys[i] = this->keys[i];
        node16->children[i] = this->children[i];
    }

    return node16;
}

// NODE 16
Node *Node16::getChildren(uint8_t partOfKey, uint8_t& indexOfLastChildrenAccessed) {
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
        indexOfLastChildrenAccessed = __builtin_ctz(bitfield);
        return this->children[indexOfLastChildrenAccessed];
    }

    return nullptr;
}

void Node16::setChildren(uint8_t partOfKey, Node *child) {
    this->keys[numberOfChildren] = partOfKey;
    this->children[numberOfChildren] = child;
    this->numberOfChildren++;
}

bool Node16::isFull() {
    return this->numberOfChildren == 16;
}

Node48 *Node16::grow() {
    auto *node48 = new Node48(this->key, this->isLeafNode);

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
Node *Node48::getChildren(uint8_t partOfKey, uint8_t& indexOfLastChildrenAccessed) {
    auto index = this->keys[partOfKey];
    // index can only be between 0 and 47 -> so if different value -> it is an error
    // might also be suitable to fill they keys before up and then just check for the ERROR_VALUE instead of this random "48"
    if (index != UNUSED_OFFSET_VALUE) {
        indexOfLastChildrenAccessed = index;
        return this->children[index];
    }
    return nullptr;
}

void Node48::setChildren(uint8_t partOfKey, Node *child) {
    this->children[numberOfChildren] = child;
    this->keys[partOfKey] = numberOfChildren;
    this->numberOfChildren++;
}

bool Node48::isFull() {
    return this->numberOfChildren == 48;
}

Node256 *Node48::grow() {
    auto node256 = new Node256(this->key, this->isLeafNode);

    node256->numberOfChildren = this->numberOfChildren;
    for (uint16_t i = 0; i < 256; i++) {
        auto index = this->keys[i];
        if (index != UNUSED_OFFSET_VALUE) {
            node256->children[i] = this->children[index];
        }
    }

    return node256;
}

// NODE 256
Node *Node256::getChildren(uint8_t partOfKey, uint8_t& indexOfLastChildrenAccessed) {
    indexOfLastChildrenAccessed = partOfKey;
    return this->children[partOfKey];
}

void Node256::setChildren(uint8_t partOfKey, Node *child) {
    this->children[partOfKey] = child;
    this->numberOfChildren++;
}

bool Node256::isFull() {
    return numberOfChildren == 256;
}
