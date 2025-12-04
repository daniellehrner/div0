#include "div0/trie/mpt.h"

#include <algorithm>
#include <cassert>

#include "div0/crypto/keccak256.h"

namespace div0::trie {

// Empty trie root: keccak256(0x80) where 0x80 is RLP of empty string
const types::Hash MerklePatriciaTrie::EMPTY_ROOT =
    types::Hash::from_hex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");

MerklePatriciaTrie::MerklePatriciaTrie(
    std::span<const std::pair<types::Bytes, types::Bytes>> items) {
  items_.reserve(items.size());
  for (const auto& [key, value] : items) {
    insert(key, value);
  }
}

void MerklePatriciaTrie::insert(std::span<const uint8_t> key, std::span<const uint8_t> value) {
  Nibbles nibbles = bytes_to_nibbles(key);
  types::Bytes val(value.begin(), value.end());

  // Check if key already exists and update
  for (auto& [existing_key, existing_value] : items_) {
    if (existing_key == nibbles) {
      existing_value = std::move(val);
      return;
    }
  }

  items_.emplace_back(std::move(nibbles), std::move(val));
}

std::optional<types::Bytes> MerklePatriciaTrie::get(std::span<const uint8_t> key) const {
  const Nibbles nibbles = bytes_to_nibbles(key);

  for (const auto& [existing_key, value] : items_) {
    if (existing_key == nibbles) {
      return value;
    }
  }
  return std::nullopt;
}

types::Hash MerklePatriciaTrie::root_hash() const {
  if (items_.empty()) {
    return EMPTY_ROOT;
  }

  if (items_.size() == 1) {
    // Single item: create leaf node
    const LeafNode leaf{.path = items_[0].first, .value = items_[0].second};
    return compute_node_hash(Node{leaf});
  }

  // Sort items by key for deterministic traversal
  std::vector<std::pair<Nibbles, types::Bytes>> sorted_items(items_);
  std::ranges::sort(sorted_items, [](const auto& a, const auto& b) { return a.first < b.first; });

  // Build tree from sorted items
  const Node root = build_node(sorted_items, 0);
  return compute_node_hash(root);
}

Node MerklePatriciaTrie::build_node(std::span<const std::pair<Nibbles, types::Bytes>> items,
                                    size_t prefix_len) const {
  assert(!items.empty());

  // Single item: leaf node
  if (items.size() == 1) {
    const auto& [key, value] = items[0];
    Nibbles remaining(key.begin() + static_cast<ptrdiff_t>(prefix_len), key.end());
    return LeafNode{.path = std::move(remaining), .value = value};
  }

  // Find common prefix among all items
  size_t common_len = 0;
  const auto& first_key = items[0].first;

  while (true) {
    const size_t pos = prefix_len + common_len;
    if (pos >= first_key.size()) {
      break;
    }

    const uint8_t nibble = first_key[pos];
    bool all_match = true;
    for (size_t i = 1; i < items.size(); ++i) {
      const auto& key = items[i].first;
      if (pos >= key.size() || key[pos] != nibble) {
        all_match = false;
        break;
      }
    }

    if (!all_match) {
      break;
    }
    ++common_len;
  }

  // If there's a common prefix, create extension node
  if (common_len > 0) {
    Nibbles shared_path(first_key.begin() + static_cast<ptrdiff_t>(prefix_len),
                        first_key.begin() + static_cast<ptrdiff_t>(prefix_len + common_len));
    NodeRef child_ref = hash_subtree(items, prefix_len + common_len);
    return ExtensionNode{.path = std::move(shared_path), .child = std::move(child_ref)};
  }

  // No common prefix: create branch node
  BranchNode branch;

  // Check if any item terminates at this position (value at branch)
  for (const auto& [key, value] : items) {
    if (key.size() == prefix_len) {
      branch.value = value;
      break;
    }
  }

  // Group items by next nibble
  for (uint8_t n = 0; n < 16; ++n) {
    std::vector<std::pair<Nibbles, types::Bytes>> child_items;

    for (const auto& [key, value] : items) {
      if (key.size() > prefix_len && key[prefix_len] == n) {
        child_items.emplace_back(key, value);
      }
    }

    if (!child_items.empty()) {
      // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) - n in [0,15],
      // children[16]
      branch.children[n] = hash_subtree(child_items, prefix_len + 1);
      // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }
  }

  return branch;
}

NodeRef MerklePatriciaTrie::hash_subtree(std::span<const std::pair<Nibbles, types::Bytes>> items,
                                         size_t prefix_len) const {
  // Need to copy span to vector for recursive call
  std::vector<std::pair<Nibbles, types::Bytes>> items_vec(items.begin(), items.end());
  const Node node = build_node(items_vec, prefix_len);
  return hash_node(node);
}

}  // namespace div0::trie
