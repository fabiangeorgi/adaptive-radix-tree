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
    if (node == nullptr) {
        return INVALID_VALUE;
    }

    Node *next = node->getChildren(key[depth]);
    // check if we are at leaf position
    if (depth == key.key_len - 1) {
        return reinterpret_cast<Value>(next);
    }
    return recursiveLookUp(next, key, depth + 1);
}

bool ART::insert(const Key &key, Value value) {
    return recursiveInsert(nullptr, root, key, value, 0);
}

void ART::replaceNode(Node *newNode, Node *parentNode) {
    if (parentNode == nullptr) { // we need to set it as root
        root = newNode;
        return;
    }

    if (parentNode->type == NodeType::N4) {
        dynamic_cast<Node4 *>(parentNode)->children[parentNode->lastIndexOfChildAccessed] = newNode;
    } else if (parentNode->type == NodeType::N16) {
        dynamic_cast<Node16 *>(parentNode)->children[parentNode->lastIndexOfChildAccessed] = newNode;
    } else if (parentNode->type == NodeType::N48) {
        dynamic_cast<Node48 *>(parentNode)->children[parentNode->lastIndexOfChildAccessed] = newNode;
    } else if (parentNode->type == NodeType::N256) {
        dynamic_cast<Node256 *>(parentNode)->children[parentNode->lastIndexOfChildAccessed] = newNode;
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

bool ART::recursiveInsert(Node *parentNode, Node *node, const Key &key, Value value, uint8_t depth) {
    auto next = node->getChildren(key[depth]);
    if (next != nullptr) {
        return recursiveInsert(node, next, key, value, depth + 1);
    } else {
        if (node->isFull()) {
            growAndReplaceNode(parentNode, node);
        }
        Node *current = node;
        while (true) {
            bool isLeaf = depth == key.key_len - 1;
            Node *child;
            if (isLeaf) {
                child = reinterpret_cast<Node *>(value);
            } else {
                child = new Node4(false);
            }

            current->addChildren(key[depth], child);

            if (isLeaf) {
                return true;
            }

            depth++;
            current = child;
        }
    }
}

// NODE 4
Node *Node4::getChildren(uint8_t partOfKey) {
    for (uint8_t i = 0; i < this->keys.size(); i++) {
        if (this->keys[i] == partOfKey) {
            this->lastIndexOfChildAccessed = i;
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
        this->lastIndexOfChildAccessed = __builtin_ctz(bitfield);
        return this->children[this->lastIndexOfChildAccessed];
    }

    return nullptr;
//// TODO for testing locally
//    for (uint8_t i = 0; i < this->keys.size(); i++) {
//        if (this->keys[i] == partOfKey) {
//            this->lastIndexOfChildAccessed = i;
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
        this->lastIndexOfChildAccessed = index;
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
    auto node256 = new Node256(this->isLeafNode);

    node256->numberOfChildren = this->numberOfChildren;
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
    this->lastIndexOfChildAccessed = partOfKey;
    return this->children[partOfKey];
}

void Node256::addChildren(uint8_t partOfKey, Node *child) {
    this->children[partOfKey] = child;
    this->numberOfChildren++;
}

bool Node256::isFull() {
    return numberOfChildren == 256;
}
