#pragma once

#include "key.hpp"

/** These are the four node sizes as described in the paper. Do not change these values! */
enum class NodeType : uint8_t {
    N4 = 0, N16 = 1, N48 = 2, N256 = 3
};

constexpr uint8_t UNUSED_OFFSET_VALUE = 100;

// TODOS:
/*
 * - implement grow
 * - check if need to fill keys with error value for NODE 48
 * - check if need to fill children with nullptr -> otherwise might be undefined behaviour
 */

/** This is the basic node class. You are free to implement the nodes in any way you see fit. We do not require
 * anything from your implementation except the public "type".
 **/
// TODO: implement node types
class Node {
public:
    // Do not change this variable. You may alter all other code in this class.
    const NodeType type;

    // we just store the whole key because we need to handle lazy expansion
    Key key;

    /* we use "Multi-value leaves"
     * that means we don't have additional leaf layers, but rather store the values directly in
     * our children array by reinterpret_casting them to Node*
     * we can do this because our key length is always constant, so this is more performant
     */
    bool isLeafNode;

    explicit Node(NodeType type, Key key, bool isLeaf) : type{type}, key{key}, isLeafNode(isLeaf) {}

    virtual ~Node() = default;

    virtual Node *getChildren(uint8_t partOfKey) = 0;

    virtual void setChildren(uint8_t partOfKey, Node *child) = 0;

    virtual bool isFull() = 0;
};

class Node256 : public Node {
public:
    explicit Node256(Key key, bool isLeafNode) : Node(NodeType::N256, key, isLeafNode) {}

    Node *getChildren(uint8_t partOfKey) override;

    void setChildren(uint8_t partOfKey, Node *child) override;

    bool isFull() override;

    // need uint16_t because uint8_t because we want need numberOfChildren = 256 -> would not work with uint8_t
    uint16_t numberOfChildren = 0;
    // don't need keys -> because can directly map
    std::array<Node *, 256> children{};
};

class Node48 : public Node {
public:
    explicit Node48(Key key, bool isLeafNode) : Node(NodeType::N48, key, isLeafNode) {
        // we need some unused_offset_value -> only 0-47 allowed -> so we just use 100 to mark this field as not assigned
        // we do that because 0 is a valid offset -> default initialization is zero
        std::ranges::fill(keys.begin(), keys.end(), UNUSED_OFFSET_VALUE);
    }

    Node *getChildren(uint8_t partOfKey) override;

    void setChildren(uint8_t partOfKey, Node *child) override;

    bool isFull() override;

    Node256 *grow();

    uint8_t numberOfChildren = 0;
    // we do it by storing the offset in the keys
    std::array<uint8_t, 256> keys{};
    std::array<Node *, 48> children{};
};

class Node16 : public Node {
public:
    explicit Node16(Key key, bool isLeafNode) : Node(NodeType::N16, key, isLeafNode) {}

    Node *getChildren(uint8_t partOfKey) override;

    void setChildren(uint8_t partOfKey, Node *child) override;

    bool isFull() override;

    Node48 *grow();

    uint8_t numberOfChildren = 0;
    std::array<uint8_t, 16> keys{};
    std::array<Node *, 16> children{};
};


class Node4 : public Node {
public:
    explicit Node4(Key key, bool isLeafNode) : Node(NodeType::N4, key, isLeafNode) {}

    /* for inner node */
    Node *getChildren(uint8_t partOfKey) override;

    void setChildren(uint8_t partOfKey, Node *child) override;

    bool isFull() override;

    Node16 *grow();

    uint8_t numberOfChildren = 0;
    std::array<uint8_t, 4> keys{};
    std::array<Node *, 4> children{};
};

/** This is the actual ART index that you need to implement. You will need to modify this class for this task. */
class ART {
private:
    Node *root = nullptr;
    // TODO: add stuff here if needed

public:
    ART();

    ~ART();

    /**
     * insert - load `value` into the tree for `key`.
     * Returns true if insert was successful, false otherwise.
     *
     * Read the task description for assumptions you can make when implementing this method.
     */
    bool insert(const Key &key, Value value);

    bool recursiveInsert(Node *parentNode, Node *node, const Key &key, Value value, uint8_t depth);

    /**
     * lookup - search for given key k in data using the index.
     * Returns INVALID_VALUE if the entry was not found.
     *
     * Read the task description for assumptions you can make when implementing this method.
     */
    Value lookup(const Key &kry);

    /**
     * get_root - returns root node for further inspection. No need mot modify this.
     */
    Node *get_root() { return root; };

    void replaceNode(Node *newNode, Node *parentNode);

    Value recursiveLookUp(Node *node, const Key &key, uint8_t depth);

    void growAndReplaceNode(Node *parentNode, Node *node);
};
