#include "test_address.h"

#include "div0/types/address.h"

#include "unity.h"

#include <string.h>

void test_address_zero_is_zero(void) {
  address_t z = address_zero();
  TEST_ASSERT_TRUE(address_is_zero(&z));

  // Verify all bytes are zero
  for (size_t i = 0; i < ADDRESS_SIZE; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, z.bytes[i]);
  }
}

void test_address_from_bytes_works(void) {
  uint8_t data[ADDRESS_SIZE];
  for (size_t i = 0; i < ADDRESS_SIZE; i++) {
    data[i] = (uint8_t)(i + 1);
  }

  address_t addr = address_from_bytes(data);
  TEST_ASSERT_FALSE(address_is_zero(&addr));
  TEST_ASSERT_EQUAL_MEMORY(data, addr.bytes, ADDRESS_SIZE);
}

void test_address_equal_works(void) {
  uint8_t data1[ADDRESS_SIZE] = {0};
  uint8_t data2[ADDRESS_SIZE] = {0};
  data1[0] = 0x42;
  data2[0] = 0x42;

  address_t a = address_from_bytes(data1);
  address_t b = address_from_bytes(data2);
  address_t c = address_zero();

  TEST_ASSERT_TRUE(address_equal(&a, &b));
  TEST_ASSERT_FALSE(address_equal(&a, &c));
}

void test_address_to_uint256_roundtrip(void) {
  // Create address with known pattern
  uint8_t data[ADDRESS_SIZE];
  for (size_t i = 0; i < ADDRESS_SIZE; i++) {
    data[i] = (uint8_t)(0x14 - i); // 0x14 = 20, descending
  }

  address_t original = address_from_bytes(data);

  // Convert to uint256 and back
  uint256_t u = address_to_uint256(&original);
  address_t restored = address_from_uint256(&u);

  TEST_ASSERT_TRUE(address_equal(&original, &restored));
}

void test_address_from_uint256_truncates(void) {
  // Create uint256 with all limbs set
  uint256_t full = uint256_from_limbs(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL,
                                      0x1112131415161718ULL, 0x191A1B1C1D1E1F20ULL);

  // Convert to address (should only keep lower 160 bits)
  address_t addr = address_from_uint256(&full);

  // Expected: limbs[2] lower 32 bits (0x15161718) + limbs[1] + limbs[0]
  // bytes[0..3] = 0x15, 0x16, 0x17, 0x18
  TEST_ASSERT_EQUAL_UINT8(0x15, addr.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0x16, addr.bytes[1]);
  TEST_ASSERT_EQUAL_UINT8(0x17, addr.bytes[2]);
  TEST_ASSERT_EQUAL_UINT8(0x18, addr.bytes[3]);

  // bytes[4..11] = limbs[1] = 0x090A0B0C0D0E0F10
  TEST_ASSERT_EQUAL_UINT8(0x09, addr.bytes[4]);
  TEST_ASSERT_EQUAL_UINT8(0x0A, addr.bytes[5]);
  TEST_ASSERT_EQUAL_UINT8(0x0B, addr.bytes[6]);
  TEST_ASSERT_EQUAL_UINT8(0x0C, addr.bytes[7]);
  TEST_ASSERT_EQUAL_UINT8(0x0D, addr.bytes[8]);
  TEST_ASSERT_EQUAL_UINT8(0x0E, addr.bytes[9]);
  TEST_ASSERT_EQUAL_UINT8(0x0F, addr.bytes[10]);
  TEST_ASSERT_EQUAL_UINT8(0x10, addr.bytes[11]);

  // bytes[12..19] = limbs[0] = 0x0102030405060708
  TEST_ASSERT_EQUAL_UINT8(0x01, addr.bytes[12]);
  TEST_ASSERT_EQUAL_UINT8(0x02, addr.bytes[13]);
  TEST_ASSERT_EQUAL_UINT8(0x03, addr.bytes[14]);
  TEST_ASSERT_EQUAL_UINT8(0x04, addr.bytes[15]);
  TEST_ASSERT_EQUAL_UINT8(0x05, addr.bytes[16]);
  TEST_ASSERT_EQUAL_UINT8(0x06, addr.bytes[17]);
  TEST_ASSERT_EQUAL_UINT8(0x07, addr.bytes[18]);
  TEST_ASSERT_EQUAL_UINT8(0x08, addr.bytes[19]);

  // Verify that converting back to uint256 has limbs[3] = 0 (truncated)
  uint256_t back = address_to_uint256(&addr);
  TEST_ASSERT_EQUAL_UINT64(0, back.limbs[3]);
  // And upper 32 bits of limbs[2] should be 0
  TEST_ASSERT_EQUAL_UINT64(0x15161718ULL, back.limbs[2]);
}