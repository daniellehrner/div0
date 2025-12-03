# RLP (Recursive Length Prefix) Implementation

High-performance RLP encoding/decoding for Ethereum data structures.

## Two-Phase Encoding

The encoder uses a two-phase approach to avoid reallocations:

1. **Phase 1**: Calculate the exact encoded length
2. **Phase 2**: Encode into a pre-allocated buffer

This design eliminates dynamic allocations during encoding, which is critical for performance in hot paths like transaction serialization.

```cpp
// Phase 1: Calculate length
const size_t len = RlpEncoder::encoded_length(my_bytes);

// Allocate once
std::vector<uint8_t> buffer(len);

// Phase 2: Encode
RlpEncoder encoder(buffer);
encoder.encode(my_bytes);
```

## Why Two Phases?

Traditional RLP encoders either:
- Reallocate as they go (slow, fragmented memory)
- Over-allocate then shrink (wasteful, extra copy)

Two-phase encoding provides:
- **Zero reallocations**: Buffer is perfectly sized upfront
- **Cache-friendly**: Single contiguous write
- **Predictable performance**: No hidden allocations

## Usage Examples

### Encoding Primitives

```cpp
#include "div0/rlp/rlp.h"

// Encode a uint64
uint64_t value = 1000;
size_t len = RlpEncoder::encoded_length(value);
std::vector<uint8_t> buf(len);
RlpEncoder encoder(buf);
encoder.encode(value);
// Result: [0x82, 0x03, 0xe8]

// Encode bytes
std::array<uint8_t, 3> data = {'d', 'o', 'g'};
len = RlpEncoder::encoded_length(data);
buf.resize(len);
RlpEncoder enc2(buf);
enc2.encode(data);
// Result: [0x83, 'd', 'o', 'g']
```

### Encoding Lists

Lists require calculating the payload length first:

```cpp
// Encode ["dog", "cat"]
auto dog = /* encoded "dog" */;  // 4 bytes
auto cat = /* encoded "cat" */;  // 4 bytes

size_t payload_len = dog.size() + cat.size();  // 8
size_t total_len = RlpEncoder::list_length(payload_len);  // 9

std::vector<uint8_t> buf(total_len);
RlpEncoder encoder(buf);
encoder.start_list(payload_len);
encoder.encode(/* "dog" bytes */);
encoder.encode(/* "cat" bytes */);
```

### Decoding

The decoder provides zero-copy access to byte strings:

```cpp
#include "div0/rlp/rlp.h"

std::vector<uint8_t> encoded = /* RLP data */;
RlpDecoder decoder(encoded);

// Decode a string (zero-copy span into input)
auto result = decoder.decode_bytes();
if (result.ok()) {
    std::span<const uint8_t> data = result.value;
}

// Decode integers
auto uint_result = decoder.decode_uint64();
auto u256_result = decoder.decode_uint256();

// Decode lists
auto list_header = decoder.decode_list_header();
if (list_header.ok()) {
    size_t payload_length = list_header.value;
    // Now decode each item in the list
}
```

## RLP Encoding Rules

| Data | Encoding |
|------|----------|
| Single byte 0x00-0x7f | As-is |
| String 0-55 bytes | `0x80 + len` + data |
| String 56+ bytes | `0xb7 + len_of_len` + len + data |
| List 0-55 bytes payload | `0xc0 + len` + items |
| List 56+ bytes payload | `0xf7 + len_of_len` + len + items |

## Files

- `helpers.h` - Constants and utility functions
- `encode.h/cpp` - Two-phase encoder
- `decode.h/cpp` - Zero-copy decoder