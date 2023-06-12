#pragma once

#include "key.hpp"

/** These are the four node sizes as described in the paper. Do not change these values! */
enum class NodeType : uint8_t {
    N4 = 0, N16 = 1, N48 = 2, N256 = 3
};

constexpr uint8_t UNUSED_OFFSET_VALUE = 255;

/** This is the basic node class. You are free to implement the nodes in any way you see fit. We do not require
 * anything from your implementation except the public "type".
 **/
class Node {
public:
    // Do not change this variable. You may alter all other code in this class.
    const NodeType type;

    bool isLeafNode;

    uint16_t numberOfChildren = 0;

    static uint8_t indexOfChildLastAccessed;

    std::array<uint8_t, 8> prefix{};

    uint8_t prefixLength;

    explicit Node(NodeType type, bool isLeaf) : type{type}, isLeafNode(isLeaf) {}

    uint8_t checkPrefix(const Key &key, uint8_t const &depth) {
        int idx = 0;
        for (; idx < this->prefixLength; idx++) {
            if (this->prefix[idx] != key[depth + idx])
                return idx;
        }
        return idx;
    }

    virtual ~Node() = default;

    virtual Node *getChildren(uint8_t const &partOfKey) = 0;

    virtual void addChildren(uint8_t const &partOfKey, Node *child) = 0;

    virtual bool isFull() = 0;
};

class LeafNode : public Node {
public:
    Key key;

    // does not use prefix or prefixlength
    Value value;

    explicit LeafNode(Key key, Value value) : Node(NodeType::N4, true), key(key), value(value) {}

    Value getValue() const {
        return value;
    }

    // we don't need those
    [[gnu::unused]] Node *getChildren(uint8_t const &partOfKey) override { return nullptr; }

    [[gnu::unused]] void addChildren(uint8_t const &partOfKey, Node *child) override {
        // noop
    };

    [[gnu::unused]] bool isFull() override { return true; };
};

class Node256 : public Node {
public:
    explicit Node256() : Node(NodeType::N256, false) {}

    Node *getChildren(uint8_t const &partOfKey) override;

    void addChildren(uint8_t const &partOfKey, Node *child) override;

    bool isFull() override;

    // don't need keys -> because can directly map
    std::array<Node *, 256> children{};
};

class Node48 : public Node {
public:
    explicit Node48() : Node(NodeType::N48, false) {
        // we need some unused_offset_value -> only 0-47 allowed -> so we just use 100 to mark this field as not assigned
        // we do that because 0 is a valid offset -> default initialization is zero
        std::ranges::fill(keys.begin(), keys.end(), UNUSED_OFFSET_VALUE);
    }

    Node *getChildren(uint8_t const &partOfKey) override;

    void addChildren(uint8_t const &partOfKey, Node *child) override;

    bool isFull() override;

    Node256 *grow();

    // we do it by storing the offset in the keys
    std::array<uint8_t, 256> keys{};
    std::array<Node *, 48> children{};
};

class Node16 : public Node {
public:
    explicit Node16() : Node(NodeType::N16, false) {}

    Node *getChildren(uint8_t const &partOfKey) override;

    void addChildren(uint8_t const &partOfKey, Node *child) override;

    bool isFull() override;

    Node48 *grow();

    std::array<uint8_t, 16> keys{};
    std::array<Node *, 16> children{};
};


class Node4 : public Node {
public:
    explicit Node4() : Node(NodeType::N4, false) {}

    Node *getChildren(uint8_t const &partOfKey) override;

    void addChildren(uint8_t const &partOfKey, Node *child) override;

    bool isFull() override;

    Node16 *grow();

    std::array<uint8_t, 4> keys{};
    std::array<Node *, 4> children{};
};

/** This is the actual ART index that you need to implement. You will need to modify this class for this task. */
class ART {
private:
    Node *root = nullptr;

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

    /**
     * lookup - search for given key k in data using the index.
     * Returns INVALID_VALUE if the entry was not found.
     *
     * Read the task description for assumptions you can make when implementing this method.
     */
    Value lookup(const Key &key);

    /**
     * get_root - returns root node for further inspection. No need mot modify this.
     */
    Node *get_root() { return root; };

    void growAndReplaceNode(Node *parentNode, Node *&node);

    void replaceNode(Node *newNode, Node *parentNode);
};
