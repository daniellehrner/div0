#include "div0/trie/node.h"

#include <cassert>
#include <cstring>

#include "div0/crypto/keccak256.h"
#include "div0/rlp/encode.h"
#include "div0/trie/hex_prefix.h"

namespace div0::trie {

namespace {

/// Encode a NodeRef to RLP bytes.
/// - Empty Bytes: encodes as empty string (0x80)
/// - Non-empty Bytes: encodes as-is (already RLP)
/// - Zero Hash: encodes as empty string (0x80)
/// - Non-zero Hash: encodes as 32-byte string
[[nodiscard]] size_t node_ref_encoded_length(const NodeRef& ref) noexcept {
  if (const auto* bytes = std::get_if<types::Bytes>(&ref)) {
    if (bytes->empty()) {
      return 1;  // 0x80 for empty string
    }
    // Already RLP-encoded, return as-is
    return bytes->size();
  }
  const auto* hash = std::get_if<types::Hash>(&ref);
  assert(hash != nullptr && "NodeRef must be Bytes or Hash");
  if (hash->is_zero()) {
    return 1;  // 0x80 for empty/null
  }
  // 32-byte string: 0xa0 + 32 bytes
  return 33;
}

/// Write a NodeRef to RLP output.
void encode_node_ref(rlp::RlpEncoder& encoder, std::span<uint8_t> buf, size_t& pos,
                     const NodeRef& ref) {
  std::visit(
      [&](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, types::Bytes>) {
          if (val.empty()) {
            buf[pos++] = 0x80;  // Empty string
          } else {
            // Already RLP-encoded, copy as-is
            std::memcpy(&buf[pos], val.data(), val.size());
            pos += val.size();
          }
        } else {
          // Hash
          if (val.is_zero()) {
            buf[pos++] = 0x80;  // Empty/null
          } else {
            // 32-byte string
            buf[pos++] = 0xa0;  // 0x80 + 32
            std::memcpy(&buf[pos], val.data(), 32);
            pos += 32;
          }
        }
      },
      ref);
  (void)encoder;  // Encoder not used for raw writing
}

}  // namespace

types::Bytes encode_node(const Node& node) {
  return std::visit(
      [](const auto& n) -> types::Bytes {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, LeafNode>) {
          // Leaf: [hex_prefix(path, is_leaf=true), value]
          const auto encoded_path = hex_prefix_encode(n.path, true);
          const size_t path_len = rlp::RlpEncoder::encoded_length(encoded_path);
          const size_t value_len = rlp::RlpEncoder::encoded_length(n.value);
          const size_t payload_len = path_len + value_len;
          const size_t total_len = rlp::RlpEncoder::list_length(payload_len);

          types::Bytes result(total_len);
          rlp::RlpEncoder encoder(result);
          encoder.start_list(payload_len);
          encoder.encode(encoded_path);
          encoder.encode(n.value);
          return result;

        } else if constexpr (std::is_same_v<T, ExtensionNode>) {
          // Extension: [hex_prefix(path, is_leaf=false), child_ref]
          const auto encoded_path = hex_prefix_encode(n.path, false);
          const size_t path_len = rlp::RlpEncoder::encoded_length(encoded_path);
          const size_t child_len = node_ref_encoded_length(n.child);
          const size_t payload_len = path_len + child_len;
          const size_t total_len = rlp::RlpEncoder::list_length(payload_len);

          types::Bytes result(total_len);
          rlp::RlpEncoder encoder(result);
          encoder.start_list(payload_len);
          encoder.encode(encoded_path);

          size_t pos = encoder.position();
          encode_node_ref(encoder, result, pos, n.child);
          return result;

        } else {
          // Branch: [child0, ..., child15, value]
          size_t payload_len = 0;
          for (const auto& child : n.children) {
            payload_len += node_ref_encoded_length(child);
          }
          payload_len += rlp::RlpEncoder::encoded_length(n.value);

          const size_t total_len = rlp::RlpEncoder::list_length(payload_len);

          types::Bytes result(total_len);
          rlp::RlpEncoder encoder(result);
          encoder.start_list(payload_len);

          size_t pos = encoder.position();
          for (const auto& child : n.children) {
            encode_node_ref(encoder, result, pos, child);
          }

          // Encode value using encoder (positioned at pos)
          if (n.value.empty()) {
            result[pos++] = 0x80;
          } else {
            // Re-create encoder at current position for value
            rlp::RlpEncoder value_encoder(std::span<uint8_t>(result).subspan(pos));
            value_encoder.encode(n.value);
          }

          return result;
        }
      },
      node);
}

NodeRef hash_node(const Node& node) {
  auto encoded = encode_node(node);

  if (encoded.size() < 32) {
    // Embed: return raw RLP bytes
    return encoded;
  }

  // Hash: return keccak256
  return crypto::keccak256(encoded);
}

types::Hash compute_node_hash(const Node& node) {
  const auto encoded = encode_node(node);
  return crypto::keccak256(encoded);
}

size_t count_children(const BranchNode& branch) noexcept {
  size_t count = 0;
  for (const auto& child : branch.children) {
    if (!is_null_ref(child)) {
      ++count;
    }
  }
  return count;
}

}  // namespace div0::trie
