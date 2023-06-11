#include "art.hpp"

#include <iostream>
#include <sstream>
#include <iterator>
#include "immintrin.h"

// TODO: implement if needed
ART::ART() = default;

// TODO: implement if needed
ART::~ART() = default;

Value ART::lookup(const Key &key) {
    return recursiveLookUp(root, key, 0);
}

Value ART::recursiveLookUp(Node *node, const Key &key, uint8_t depth) {
    if (node == nullptr) {
        return INVALID_VALUE;
    }

    if (node->isLeafNode) {
        // leaf matches
        if (0 == std::memcmp(&node->key, &key, depth)) {
            // something is not right here
            return dynamic_cast<LeafNode*>(node)->getValue();
        } else {
            return INVALID_VALUE;
        }
    }

    if (node->checkPrefix(key, depth) != node->prefixLength) {
        return INVALID_VALUE;
    }

    depth = depth + node->prefixLength;
    auto *next = node->getChildren(key[depth]);
    return recursiveLookUp(next, key, depth + 1);
}

bool ART::insert(const Key &key, Value value) {
    auto leaf = new LeafNode(key, value);
    // we need to store the last key information -> this is identifier for this particular node
    // we still save the whole key in the node, so we can reinterpret the path

    return recursiveInsert(nullptr, root, key, leaf, 0);
}

void ART::replaceNode(Node *newNode, Node *parentNode) {
    if (parentNode == nullptr) { // we need to set it as root
        root = newNode;
        return;
    }

    if (parentNode->type == NodeType::N4) {
        dynamic_cast<Node4 *>(parentNode)->children[parentNode->indexOfChildLastAccessed] = newNode;
    } else if (parentNode->type == NodeType::N16) {
        dynamic_cast<Node16 *>(parentNode)->children[parentNode->indexOfChildLastAccessed] = newNode;
    } else if (parentNode->type == NodeType::N48) {
        dynamic_cast<Node48 *>(parentNode)->children[parentNode->indexOfChildLastAccessed] = newNode;
    } else if (parentNode->type == NodeType::N256) {
        dynamic_cast<Node256 *>(parentNode)->children[parentNode->indexOfChildLastAccessed] = newNode;
    }
}

void ART::growAndReplaceNode(Node *parentNode, Node *&node) {
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

bool ART::recursiveInsert(Node *parentNode, Node *node, const Key &key, Node *leaf, uint8_t depth) {
    if (node == nullptr) { // handle empty tree case
        // set as new root
        root = leaf;
        return true;
    }

    if (node->isLeafNode) {
        auto newNode = new Node4(key, false);
        auto const &key2 = node->key;

        size_t i = depth;
        for (; key[i] == key2[i]; i = i + 1) {
            newNode->prefix[i - depth] = key[i];
        }
        newNode->prefixLength = i - depth;

        depth = depth + newNode->prefixLength;
        newNode->addChildren(key[depth], leaf);
        newNode->addChildren(key2[depth], node);

        replaceNode(newNode, parentNode);
        return true;
    }
    uint8_t p = node->checkPrefix(key, depth);
    if (p != node->prefixLength) {
        auto newNode = new Node4(key, false);
        newNode->addChildren(key[depth + p], leaf);
        newNode->addChildren(node->prefix[p], node);
        newNode->prefixLength = p;
        std::memcpy(&newNode->prefix, &node->prefix, p);
        node->prefixLength = node->prefixLength - (p + 1);
        std::memmove(&node->prefix, &node->prefix + p + 1, node->prefixLength);
        replaceNode(newNode, parentNode);
        return true;
    }
    depth = depth + node->prefixLength;
    auto *next = node->getChildren(key[depth]);
    if (next != nullptr) {
        return recursiveInsert(node, next, key, leaf, depth + 1);
    } else {
        if (node->isFull()) {
            growAndReplaceNode(parentNode, node);
        }
        node->addChildren(key[depth], leaf);
        return true;
    }
}

// NODE 4
Node *Node4::getChildren(uint8_t partOfKey) {
    for (uint8_t i = 0; i < this->keys.size(); i++) {
        if (this->keys[i] == partOfKey) {
            this->indexOfChildLastAccessed = i;
            return this->children[i];
        }
    }
    return nullptr;
}

void Node4::addChildren(uint8_t partOfKey, Node *child) {
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
    node16->prefix = this->prefix;
    node16->prefixLength = this->prefixLength;
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
        this->indexOfChildLastAccessed = __builtin_ctz(bitfield);
        return this->children[this->indexOfChildLastAccessed];
    }

    return nullptr;
// TODO for testing locally
//    for (uint8_t i = 0; i < this->keys.size(); i++) {
//        if (this->keys[i] == partOfKey) {
//            this->indexOfChildLastAccessed = i;
//            return this->children[i];
//        }
//    }
//    return nullptr;
}

void Node16::addChildren(uint8_t partOfKey, Node *child) {
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
    node48->prefix = this->prefix;
    node48->prefixLength = this->prefixLength;
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
        this->indexOfChildLastAccessed = index;
        return this->children[index];
    }
    return nullptr;
}

void Node48::addChildren(uint8_t partOfKey, Node *child) {
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
    node256->prefix = this->prefix;
    node256->prefixLength = this->prefixLength;
    for (uint16_t i = 0; i < 256; i++) {
        auto index = this->keys[i];
        if (index != UNUSED_OFFSET_VALUE) {
            node256->children[i] = this->children[index];
        } else {
            node256->children[i] = nullptr;
        }
    }

    return node256;
}

// NODE 256
Node *Node256::getChildren(uint8_t partOfKey) {
    this->indexOfChildLastAccessed = partOfKey;
    return this->children[partOfKey];
}

void Node256::addChildren(uint8_t partOfKey, Node *child) {
    this->children[partOfKey] = child;
    this->numberOfChildren++;
}

bool Node256::isFull() {
    return numberOfChildren == 256;
}
