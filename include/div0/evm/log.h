#ifndef DIV0_EVM_LOG_H
#define DIV0_EVM_LOG_H

#include "div0/types/address.h"
#include "div0/types/hash.h"

#include <stddef.h>

/// Maximum topics per log (LOG4 has 4 topics).
static constexpr size_t LOG_MAX_TOPICS = 4;

/// EVM log entry (event emitted during execution).
typedef struct {
  address_t address;             // Contract that emitted the log
  hash_t topics[LOG_MAX_TOPICS]; // Topic array (0-4 topics)
  size_t topic_count;            // Number of topics (0-4)
  const uint8_t *data;           // Log data (arena-allocated)
  size_t data_size;              // Size of data in bytes
} evm_log_t;

#endif // DIV0_EVM_LOG_H
