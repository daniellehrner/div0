#include "div0/ethereum/account.h"

#include "div0/rlp/encode.h"

namespace div0::ethereum {

// Empty code hash: keccak256("")
// = 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
const types::Hash EMPTY_CODE_HASH =
    types::Hash::from_hex("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");

types::Bytes rlp_encode_account(const AccountState& account) {
  // Account encoding: [nonce, balance, storage_root, code_hash]
  // - nonce: uint64 (RLP integer)
  // - balance: Uint256 (RLP integer, big-endian, no leading zeros)
  // - storage_root: 32 bytes (RLP string)
  // - code_hash: 32 bytes (RLP string)

  // Calculate payload length
  const size_t nonce_len = rlp::RlpEncoder::encoded_length(account.nonce);
  const size_t balance_len = rlp::RlpEncoder::encoded_length(account.balance);
  const size_t storage_root_len = rlp::RlpEncoder::encoded_length(account.storage_root.span());
  const size_t code_hash_len = rlp::RlpEncoder::encoded_length(account.code_hash.span());

  const size_t payload_len = nonce_len + balance_len + storage_root_len + code_hash_len;
  const size_t total_len = rlp::RlpEncoder::list_length(payload_len);

  // Encode
  types::Bytes result(total_len);
  rlp::RlpEncoder encoder(result);

  encoder.start_list(payload_len);
  encoder.encode(account.nonce);
  encoder.encode(account.balance);
  encoder.encode(account.storage_root.span());
  encoder.encode(account.code_hash.span());

  return result;
}

}  // namespace div0::ethereum
