#pragma once

#include "key.hpp"

/** These are the four node sizes as described in the paper. Do not change these values! */
enum class NodeType : uint8_t { N4 = 0, N16 = 1, N48 = 2, N256 = 3 };

/** This is the basic node class. You are free to implement the nodes in any way you see fit. We do not require
 * anything from your implementation except the public "type".
 **/
// TODO: implement node types
class Node {
 public:
  // Do not change this variable. You may alter all other code in this class.
  const NodeType type;

  explicit Node(NodeType type) : type{type} {}
  ~Node() = default;
};

/** This is the actual ART index that you need to implement. You will need to modify this class for this task. */
class ART {
 private:
  Node* root;
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
  bool insert(const Key& key, Value value);

  /**
   * lookup - search for given key k in data using the index.
   * Returns INVALID_VALUE if the entry was not found.
   *
   * Read the task description for assumptions you can make when implementing this method.
   */
  Value lookup(const Key& kry);

  /**
   * get_root - returns root node for further inspection. No need mot modify this.
   */
  Node* get_root() { return root; };
};
