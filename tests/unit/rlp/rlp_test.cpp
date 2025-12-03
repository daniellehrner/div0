#include "div0/rlp/rlp.h"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace div0::rlp {
namespace {

// Helper to convert hex string to bytes
std::vector<uint8_t> hex_to_bytes(std::string_view hex) {
  if (hex.size() >= 2 && hex[0] == '0' && hex[1] == 'x') {
    hex = hex.substr(2);
  }
  std::vector<uint8_t> bytes;
  bytes.reserve(hex.size() / 2);
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    const auto hi = hex[i];
    const auto lo = hex[i + 1];
    auto nibble = [](char c) -> uint8_t {
      if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
      }
      if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(c - 'a' + 10);
      }
      if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(c - 'A' + 10);
      }
      return 0;
    };
    bytes.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
  }
  return bytes;
}

std::string bytes_to_hex(std::span<const uint8_t> bytes) {
  static constexpr std::array<char, 17> K_HEX_CHARS = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                                       '9', 'a', 'b', 'c', 'd', 'e', 'f', '\0'};
  std::string result = "0x";
  result.reserve((bytes.size() * 2) + 2);
  for (const auto byte : bytes) {
    result += K_HEX_CHARS.at((byte >> 4) & 0xF);
    result += K_HEX_CHARS.at(byte & 0xF);
  }
  return result;
}

class RlpEncodeTest : public ::testing::Test {
 protected:
  static std::vector<uint8_t> encode_bytes(std::span<const uint8_t> data) {
    const size_t len = RlpEncoder::encoded_length(data);
    std::vector<uint8_t> result(len);
    RlpEncoder encoder(result);
    encoder.encode(data);
    return result;
  }

  static std::vector<uint8_t> encode_uint64(uint64_t value) {
    const size_t len = RlpEncoder::encoded_length(value);
    std::vector<uint8_t> result(len);
    RlpEncoder encoder(result);
    encoder.encode(value);
    return result;
  }

  static std::vector<uint8_t> encode_uint256(const types::Uint256& value) {
    const size_t len = RlpEncoder::encoded_length(value);
    std::vector<uint8_t> result(len);
    RlpEncoder encoder(result);
    encoder.encode(value);
    return result;
  }

  static std::vector<uint8_t> encode_string(std::string_view str) {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto bytes = std::span(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    return encode_bytes(bytes);
  }
};

// ===========================================================================
// Ethereum RLP Test Vectors - Encoding
// ===========================================================================

TEST_F(RlpEncodeTest, EmptyString) { EXPECT_EQ(bytes_to_hex(encode_bytes({})), "0x80"); }

TEST_F(RlpEncodeTest, ByteString00) {
  const std::array<uint8_t, 1> data = {0x00};
  EXPECT_EQ(bytes_to_hex(encode_bytes(data)), "0x00");
}

TEST_F(RlpEncodeTest, ByteString01) {
  const std::array<uint8_t, 1> data = {0x01};
  EXPECT_EQ(bytes_to_hex(encode_bytes(data)), "0x01");
}

TEST_F(RlpEncodeTest, ByteString7F) {
  const std::array<uint8_t, 1> data = {0x7F};
  EXPECT_EQ(bytes_to_hex(encode_bytes(data)), "0x7f");
}

TEST_F(RlpEncodeTest, ShortString_Dog) {
  EXPECT_EQ(bytes_to_hex(encode_string("dog")), "0x83646f67");
}

TEST_F(RlpEncodeTest, ShortString_55Bytes) {
  // "Lorem ipsum dolor sit amet, consectetur adipisicing eli" (55 bytes)
  const std::string_view str = "Lorem ipsum dolor sit amet, consectetur adipisicing eli";
  EXPECT_EQ(str.size(), 55);
  EXPECT_EQ(bytes_to_hex(encode_string(str)),
            "0xb74c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
            "7572206164697069736963696e6720656c69");
}

TEST_F(RlpEncodeTest, LongString_56Bytes) {
  // "Lorem ipsum dolor sit amet, consectetur adipisicing elit" (56 bytes)
  const std::string_view str = "Lorem ipsum dolor sit amet, consectetur adipisicing elit";
  EXPECT_EQ(str.size(), 56);
  EXPECT_EQ(bytes_to_hex(encode_string(str)),
            "0xb8384c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
            "747572206164697069736963696e6720656c6974");
}

TEST_F(RlpEncodeTest, Zero) { EXPECT_EQ(bytes_to_hex(encode_uint64(0)), "0x80"); }

TEST_F(RlpEncodeTest, SmallInt1) { EXPECT_EQ(bytes_to_hex(encode_uint64(1)), "0x01"); }

TEST_F(RlpEncodeTest, SmallInt16) { EXPECT_EQ(bytes_to_hex(encode_uint64(16)), "0x10"); }

TEST_F(RlpEncodeTest, SmallInt79) { EXPECT_EQ(bytes_to_hex(encode_uint64(79)), "0x4f"); }

TEST_F(RlpEncodeTest, SmallInt127) { EXPECT_EQ(bytes_to_hex(encode_uint64(127)), "0x7f"); }

TEST_F(RlpEncodeTest, MediumInt128) { EXPECT_EQ(bytes_to_hex(encode_uint64(128)), "0x8180"); }

TEST_F(RlpEncodeTest, MediumInt1000) { EXPECT_EQ(bytes_to_hex(encode_uint64(1000)), "0x8203e8"); }

TEST_F(RlpEncodeTest, MediumInt100000) {
  EXPECT_EQ(bytes_to_hex(encode_uint64(100000)), "0x830186a0");
}

TEST_F(RlpEncodeTest, BigInt_15Bytes) {
  // #83729609699884896815286331701780722 = 0x102030405060708090a0b0c0d0e0f2
  // This fits in 15 bytes, limb0 has the lower 8, limb1 has the upper 7
  // 0x102030405060708090a0b0c0d0e0f2
  //   limb0 = 0x8090a0b0c0d0e0f2
  //   limb1 = 0x0010203040506070
  const auto value = types::Uint256(0x8090a0b0c0d0e0f2ULL, 0x0010203040506070ULL, 0, 0);

  EXPECT_EQ(bytes_to_hex(encode_uint256(value)), "0x8f102030405060708090a0b0c0d0e0f2");
}

TEST_F(RlpEncodeTest, BigInt_33Bytes) {
  // 2^256 = 0x010000...0000 (33 bytes including leading 01)
  // This is > max Uint256, so encode as raw bytes
  const auto bytes =
      hex_to_bytes("0x010000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(bytes_to_hex(encode_bytes(bytes)),
            "0xa1010000000000000000000000000000000000000000000000000000000000000000");
}

TEST_F(RlpEncodeTest, EmptyList) {
  std::vector<uint8_t> buf(1);
  RlpEncoder encoder(buf);
  encoder.start_list(0);
  EXPECT_EQ(bytes_to_hex(buf), "0xc0");
}

TEST_F(RlpEncodeTest, StringList_DogGodCat) {
  // ["dog", "god", "cat"] = 0xcc83646f6783676f6483636174
  const auto dog = encode_string("dog");
  const auto god = encode_string("god");
  const auto cat = encode_string("cat");

  const size_t payload_len = dog.size() + god.size() + cat.size();
  const size_t total_len = RlpEncoder::list_length(payload_len);

  std::vector<uint8_t> buf(total_len);
  RlpEncoder encoder(buf);
  encoder.start_list(payload_len);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto dog_bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("dog"), 3);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto god_bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("god"), 3);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto cat_bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("cat"), 3);

  encoder.encode(dog_bytes);
  encoder.encode(god_bytes);
  encoder.encode(cat_bytes);

  EXPECT_EQ(bytes_to_hex(buf), "0xcc83646f6783676f6483636174");
}

TEST_F(RlpEncodeTest, MultiList) {
  // ["zw", [4], 1] = 0xc6827a77c10401
  // "zw" = 0x827a77
  // [4] = 0xc104
  // 1 = 0x01
  const size_t inner_payload = 1;                                   // single byte 4
  const size_t inner_len = RlpEncoder::list_length(inner_payload);  // 2

  const size_t zw_len = 3;  // 0x82 + "zw"
  const size_t one_len = 1;
  const size_t payload_len = zw_len + inner_len + one_len;

  std::vector<uint8_t> buf(RlpEncoder::list_length(payload_len));
  RlpEncoder encoder(buf);

  encoder.start_list(payload_len);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  encoder.encode(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("zw"), 2));
  encoder.start_list(inner_payload);
  encoder.encode(static_cast<uint64_t>(4));
  encoder.encode(static_cast<uint64_t>(1));

  EXPECT_EQ(bytes_to_hex(buf), "0xc6827a77c10401");
}

TEST_F(RlpEncodeTest, ListsOfLists) {
  // [ [ [], [] ], [] ] = 0xc4c2c0c0c0
  const size_t empty_list_len = 1;
  const size_t inner_payload = empty_list_len + empty_list_len;     // 2
  const size_t inner_len = RlpEncoder::list_length(inner_payload);  // 3
  const size_t outer_payload = inner_len + empty_list_len;          // 4

  std::vector<uint8_t> buf(RlpEncoder::list_length(outer_payload));
  RlpEncoder encoder(buf);

  encoder.start_list(outer_payload);
  encoder.start_list(inner_payload);
  encoder.start_list(0);
  encoder.start_list(0);
  encoder.start_list(0);

  EXPECT_EQ(bytes_to_hex(buf), "0xc4c2c0c0c0");
}

TEST_F(RlpEncodeTest, ListsOfLists2) {
  // [ [], [[]], [ [], [[]] ] ] = 0xc7c0c1c0c3c0c1c0
  std::vector<uint8_t> buf(8);
  RlpEncoder encoder(buf);

  // Outer list payload = 1 + 2 + 4 = 7
  encoder.start_list(7);
  encoder.start_list(0);  // []
  encoder.start_list(1);  // [[]]
  encoder.start_list(0);  // inner []
  encoder.start_list(3);  // [ [], [[]] ]
  encoder.start_list(0);  // []
  encoder.start_list(1);  // [[]]
  encoder.start_list(0);  // inner []

  EXPECT_EQ(bytes_to_hex(buf), "0xc7c0c1c0c3c0c1c0");
}

// ===========================================================================
// Ethereum RLP Test Vectors - Decoding
// ===========================================================================

class RlpDecodeTest : public ::testing::Test {};

TEST_F(RlpDecodeTest, EmptyString) {
  const auto input = hex_to_bytes("0x80");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  ASSERT_TRUE(result.ok());
  EXPECT_TRUE(result.value.empty());
}

TEST_F(RlpDecodeTest, ByteString00) {
  const auto input = hex_to_bytes("0x00");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  ASSERT_TRUE(result.ok());
  ASSERT_EQ(result.value.size(), 1);
  EXPECT_EQ(result.value[0], 0x00);
}

TEST_F(RlpDecodeTest, ByteString7F) {
  const auto input = hex_to_bytes("0x7f");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  ASSERT_TRUE(result.ok());
  ASSERT_EQ(result.value.size(), 1);
  EXPECT_EQ(result.value[0], 0x7f);
}

TEST_F(RlpDecodeTest, ShortString_Dog) {
  const auto input = hex_to_bytes("0x83646f67");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  ASSERT_TRUE(result.ok());
  ASSERT_EQ(result.value.size(), 3);
  EXPECT_EQ(result.value[0], 'd');
  EXPECT_EQ(result.value[1], 'o');
  EXPECT_EQ(result.value[2], 'g');
}

TEST_F(RlpDecodeTest, Zero) {
  const auto input = hex_to_bytes("0x80");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 0);
}

TEST_F(RlpDecodeTest, SmallInt1) {
  const auto input = hex_to_bytes("0x01");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 1);
}

TEST_F(RlpDecodeTest, SmallInt127) {
  const auto input = hex_to_bytes("0x7f");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 127);
}

TEST_F(RlpDecodeTest, MediumInt128) {
  const auto input = hex_to_bytes("0x8180");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 128);
}

TEST_F(RlpDecodeTest, MediumInt1000) {
  const auto input = hex_to_bytes("0x8203e8");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 1000);
}

TEST_F(RlpDecodeTest, MediumInt100000) {
  const auto input = hex_to_bytes("0x830186a0");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint64();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 100000);
}

TEST_F(RlpDecodeTest, EmptyList) {
  const auto input = hex_to_bytes("0xc0");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_list_header();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, 0);
}

TEST_F(RlpDecodeTest, StringList_DogGodCat) {
  const auto input = hex_to_bytes("0xcc83646f6783676f6483636174");
  RlpDecoder decoder(input);

  const auto list = decoder.decode_list_header();
  ASSERT_TRUE(list.ok());
  EXPECT_EQ(list.value, 12);

  const auto dog = decoder.decode_bytes();
  ASSERT_TRUE(dog.ok());
  EXPECT_EQ(dog.value.size(), 3);

  const auto god = decoder.decode_bytes();
  ASSERT_TRUE(god.ok());
  EXPECT_EQ(god.value.size(), 3);

  const auto cat = decoder.decode_bytes();
  ASSERT_TRUE(cat.ok());
  EXPECT_EQ(cat.value.size(), 3);

  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, ListsOfLists) {
  const auto input = hex_to_bytes("0xc4c2c0c0c0");
  RlpDecoder decoder(input);

  const auto outer = decoder.decode_list_header();
  ASSERT_TRUE(outer.ok());
  EXPECT_EQ(outer.value, 4);

  const auto inner = decoder.decode_list_header();
  ASSERT_TRUE(inner.ok());
  EXPECT_EQ(inner.value, 2);

  const auto empty1 = decoder.decode_list_header();
  ASSERT_TRUE(empty1.ok());
  EXPECT_EQ(empty1.value, 0);

  const auto empty2 = decoder.decode_list_header();
  ASSERT_TRUE(empty2.ok());
  EXPECT_EQ(empty2.value, 0);

  const auto empty3 = decoder.decode_list_header();
  ASSERT_TRUE(empty3.ok());
  EXPECT_EQ(empty3.value, 0);

  EXPECT_FALSE(decoder.has_more());
}

// ===========================================================================
// Invalid RLP Test Vectors
// ===========================================================================

TEST_F(RlpDecodeTest, Invalid_BytesShouldBeSingleByte00) {
  const auto input = hex_to_bytes("0x8100");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::NonCanonical);
}

TEST_F(RlpDecodeTest, Invalid_BytesShouldBeSingleByte01) {
  const auto input = hex_to_bytes("0x8101");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::NonCanonical);
}

TEST_F(RlpDecodeTest, Invalid_BytesShouldBeSingleByte7F) {
  const auto input = hex_to_bytes("0x817F");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::NonCanonical);
}

TEST_F(RlpDecodeTest, Invalid_LeadingZerosInLongLengthArray) {
  const auto input = hex_to_bytes("0xb800");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::LeadingZeros);
}

TEST_F(RlpDecodeTest, Invalid_LeadingZerosInLongLengthList) {
  const auto input = hex_to_bytes("0xf800");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_list_header();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::LeadingZeros);
}

TEST_F(RlpDecodeTest, Invalid_NonOptimalLongLengthArray) {
  const auto input = hex_to_bytes("0xb801ff");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::NonCanonical);
}

TEST_F(RlpDecodeTest, Invalid_NonOptimalLongLengthList) {
  const auto input = hex_to_bytes("0xf803112233");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_list_header();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::NonCanonical);
}

TEST_F(RlpDecodeTest, Invalid_EmptyEncoding) {
  const std::vector<uint8_t> input;
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::InputTooShort);
}

TEST_F(RlpDecodeTest, Invalid_TruncatedShortString) {
  const auto input = hex_to_bytes("0x81");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::InputTooShort);
}

TEST_F(RlpDecodeTest, Invalid_TruncatedShortString2) {
  const auto input =
      hex_to_bytes("0xa0000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_bytes();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::InputTooShort);
}

// ===========================================================================
// Uint256 Tests
// ===========================================================================

TEST_F(RlpDecodeTest, Uint256_Zero) {
  const auto input = hex_to_bytes("0x80");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint256();
  ASSERT_TRUE(result.ok());
  EXPECT_TRUE(result.value.is_zero());
}

TEST_F(RlpDecodeTest, Uint256_One) {
  const auto input = hex_to_bytes("0x01");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint256();
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result.value, types::Uint256(1));
}

TEST_F(RlpDecodeTest, Uint256_BigInt) {
  const auto input = hex_to_bytes("0x8f102030405060708090a0b0c0d0e0f2");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint256();
  ASSERT_TRUE(result.ok());

  // 0x102030405060708090a0b0c0d0e0f2
  // limb0 = 0x8090a0b0c0d0e0f2, limb1 = 0x0010203040506070
  const auto expected = types::Uint256(0x8090a0b0c0d0e0f2ULL, 0x0010203040506070ULL, 0, 0);
  EXPECT_EQ(result.value, expected);
}

TEST_F(RlpDecodeTest, Uint256_LeadingZerosInvalid) {
  // Integer with leading zero byte is non-canonical
  const auto input = hex_to_bytes("0x82007f");
  RlpDecoder decoder(input);
  const auto result = decoder.decode_uint256();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::LeadingZeros);
}

// ===========================================================================
// Address Tests
// ===========================================================================

TEST_F(RlpDecodeTest, Address_Valid) {
  // Create input: 0x94 prefix + 20 bytes (0x01, 0x02, ..., 0x14)
  std::vector<uint8_t> input(21);
  input[0] = 0x94;
  for (size_t i = 0; i < 20; ++i) {
    input[i + 1] = static_cast<uint8_t>(i + 1);
  }

  RlpDecoder decoder(input);
  const auto result = decoder.decode_address();
  ASSERT_TRUE(result.ok());

  // Construct expected address from the same bytes
  std::array<uint8_t, 20> addr_bytes{};
  for (size_t i = 0; i < 20; ++i) {
    addr_bytes.at(i) = static_cast<uint8_t>(i + 1);
  }
  const std::span<const uint8_t, 20> addr_span{addr_bytes};
  const types::Address expected{addr_span};
  EXPECT_EQ(result.value, expected);
}

TEST_F(RlpDecodeTest, Address_WrongSize19) {
  std::vector<uint8_t> input(20);
  input[0] = 0x93;  // 19 bytes
  RlpDecoder decoder(input);
  const auto result = decoder.decode_address();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::InvalidPrefix);
}

TEST_F(RlpDecodeTest, Address_WrongSize21) {
  std::vector<uint8_t> input(22);
  input[0] = 0x95;  // 21 bytes
  RlpDecoder decoder(input);
  const auto result = decoder.decode_address();
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error, DecodeError::InvalidPrefix);
}

// ===========================================================================
// Roundtrip Tests
// ===========================================================================

TEST_F(RlpDecodeTest, Roundtrip_Bytes) {
  const std::vector<std::vector<uint8_t>> test_cases = {
      {},
      {0x00},
      {0x7f},
      {0x80},
      {0xff},
      {0x01, 0x02, 0x03},
      std::vector<uint8_t>(55, 0xAB),
      std::vector<uint8_t>(56, 0xCD),
      std::vector<uint8_t>(256, 0xEF),
  };

  for (const auto& original : test_cases) {
    const size_t len = RlpEncoder::encoded_length(original);
    std::vector<uint8_t> encoded(len);
    RlpEncoder encoder(encoded);
    encoder.encode(original);

    RlpDecoder decoder(encoded);
    const auto result = decoder.decode_bytes();

    ASSERT_TRUE(result.ok()) << "Failed for size " << original.size();
    EXPECT_EQ(std::vector<uint8_t>(result.value.begin(), result.value.end()), original);
  }
}

TEST_F(RlpDecodeTest, Roundtrip_Uint64) {
  const std::vector<uint64_t> test_cases = {
      0, 1, 127, 128, 255, 256, 1000, 100000, UINT32_MAX, UINT64_MAX,
  };

  for (const uint64_t original : test_cases) {
    const size_t len = RlpEncoder::encoded_length(original);
    std::vector<uint8_t> encoded(len);
    RlpEncoder encoder(encoded);
    encoder.encode(original);

    RlpDecoder decoder(encoded);
    const auto result = decoder.decode_uint64();

    ASSERT_TRUE(result.ok()) << "Failed for " << original;
    EXPECT_EQ(result.value, original);
  }
}

TEST_F(RlpDecodeTest, Roundtrip_Uint256) {
  const std::vector<types::Uint256> test_cases = {
      types::Uint256{},      types::Uint256(1),          types::Uint256(127),
      types::Uint256(128),   types::Uint256(UINT64_MAX),
      types::Uint256::max(),  // All 0xFF bytes
  };

  for (const auto& original : test_cases) {
    const size_t len = RlpEncoder::encoded_length(original);
    std::vector<uint8_t> encoded(len);
    RlpEncoder encoder(encoded);
    encoder.encode(original);

    RlpDecoder decoder(encoded);
    const auto result = decoder.decode_uint256();

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value, original);
  }
}

// ===========================================================================
// skip_item bounds checking tests
// ===========================================================================

TEST_F(RlpDecodeTest, SkipItem_SingleByte) {
  // Valid single byte - should advance by 1
  const std::vector<uint8_t> data = {0x42};
  RlpDecoder decoder(data);
  decoder.skip_item();
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_ShortString) {
  // Valid short string "dog" (0x83 + 3 bytes)
  const std::vector<uint8_t> data = {0x83, 'd', 'o', 'g'};
  RlpDecoder decoder(data);
  decoder.skip_item();
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_ShortStringTruncated) {
  // Short string prefix claims 10 bytes but only 2 available
  // 0x8a = 0x80 + 10 (claims 10 bytes follow)
  const std::vector<uint8_t> data = {0x8a, 'a', 'b'};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should clamp to end, not go past buffer
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_LongStringTruncated) {
  // Long string: 0xb8 0x40 claims 64 bytes follow, but only 3 available
  const std::vector<uint8_t> data = {0xb8, 0x40, 'a', 'b', 'c'};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should clamp to end, not go past buffer
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_LongStringTruncatedLength) {
  // Long string: 0xb9 claims 2-byte length follows, but only 1 byte available
  const std::vector<uint8_t> data = {0xb9, 0x01};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should handle gracefully
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_ShortListTruncated) {
  // Short list: 0xc5 = 0xc0 + 5 (claims 5 bytes payload)
  const std::vector<uint8_t> data = {0xc5, 0x01, 0x02};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should clamp to end, not go past buffer
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_LongListTruncated) {
  // Long list: 0xf8 0x80 claims 128 bytes payload, but only a few available
  const std::vector<uint8_t> data = {0xf8, 0x80, 0x01, 0x02, 0x03};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should clamp to end, not go past buffer
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_LongListTruncatedLength) {
  // Long list: 0xf9 claims 2-byte length follows, but only 1 byte available
  const std::vector<uint8_t> data = {0xf9, 0x01};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should handle gracefully
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_EmptyInput) {
  // Empty input - skip_item should do nothing
  const std::vector<uint8_t> data = {};
  RlpDecoder decoder(data);
  decoder.skip_item();
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_MultipleItems) {
  // Multiple valid items: [0x01, 0x02, "dog"]
  const std::vector<uint8_t> data = {0x01, 0x02, 0x83, 'd', 'o', 'g'};
  RlpDecoder decoder(data);

  decoder.skip_item();  // skip 0x01
  EXPECT_TRUE(decoder.has_more());

  decoder.skip_item();  // skip 0x02
  EXPECT_TRUE(decoder.has_more());

  decoder.skip_item();  // skip "dog"
  EXPECT_FALSE(decoder.has_more());
}

TEST_F(RlpDecodeTest, SkipItem_MassiveLength) {
  // Short string with prefix claiming maximum short length (55 bytes)
  // 0xb7 = 0x80 + 55, but only 2 bytes follow
  const std::vector<uint8_t> data = {0xb7, 'a', 'b'};
  RlpDecoder decoder(data);
  decoder.skip_item();
  // Should clamp to end
  EXPECT_FALSE(decoder.has_more());
}

// ===========================================================================
// Helper function tests
// ===========================================================================

TEST(RlpHelpersTest, PrefixLength) {
  EXPECT_EQ(prefix_length(0x00), 0);  // single byte
  EXPECT_EQ(prefix_length(0x7f), 0);  // single byte
  EXPECT_EQ(prefix_length(0x80), 1);  // short string
  EXPECT_EQ(prefix_length(0xb7), 1);  // short string max
  EXPECT_EQ(prefix_length(0xb8), 2);  // long string, 1 byte len
  EXPECT_EQ(prefix_length(0xbf), 9);  // long string, 8 byte len
  EXPECT_EQ(prefix_length(0xc0), 1);  // short list
  EXPECT_EQ(prefix_length(0xf7), 1);  // short list max
  EXPECT_EQ(prefix_length(0xf8), 2);  // long list, 1 byte len
  EXPECT_EQ(prefix_length(0xff), 9);  // long list, 8 byte len
}

TEST(RlpHelpersTest, LengthOfLength) {
  EXPECT_EQ(length_of_length(0), 0);
  EXPECT_EQ(length_of_length(55), 0);
  EXPECT_EQ(length_of_length(56), 1);
  EXPECT_EQ(length_of_length(255), 1);
  EXPECT_EQ(length_of_length(256), 2);
  EXPECT_EQ(length_of_length(65535), 2);
  EXPECT_EQ(length_of_length(65536), 3);
}

TEST(RlpHelpersTest, ByteLength) {
  EXPECT_EQ(byte_length(0), 0);
  EXPECT_EQ(byte_length(1), 1);
  EXPECT_EQ(byte_length(255), 1);
  EXPECT_EQ(byte_length(256), 2);
  EXPECT_EQ(byte_length(65535), 2);
  EXPECT_EQ(byte_length(65536), 3);
  EXPECT_EQ(byte_length(UINT64_MAX), 8);
}

TEST(RlpHelpersTest, IsStringPrefix) {
  EXPECT_TRUE(is_string_prefix(0x00));
  EXPECT_TRUE(is_string_prefix(0x7f));
  EXPECT_TRUE(is_string_prefix(0x80));
  EXPECT_TRUE(is_string_prefix(0xbf));
  EXPECT_FALSE(is_string_prefix(0xc0));
  EXPECT_FALSE(is_string_prefix(0xff));
}

TEST(RlpHelpersTest, IsListPrefix) {
  EXPECT_FALSE(is_list_prefix(0x00));
  EXPECT_FALSE(is_list_prefix(0xbf));
  EXPECT_TRUE(is_list_prefix(0xc0));
  EXPECT_TRUE(is_list_prefix(0xff));
}

}  // namespace
}  // namespace div0::rlp
