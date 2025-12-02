#include <div0/types/uint256.h>

namespace div0::types {

Uint256 Uint256::exp(const Uint256& base, const Uint256& exponent) noexcept {
  // Special cases for quick exit
  if (exponent.is_zero()) {
    return Uint256(1);  // x^0 = 1
  }

  if (base.is_zero()) {
    return Uint256(0);  // 0^n = 0 (for n > 0)
  }

  if (base == Uint256(1)) {
    return Uint256(1);  // 1^n = 1
  }

  // Optimization: if base is even and exponent is large, result will overflow to 0
  // An even base raised to power >= 256 will be 0 mod 2^256
  // (2^k)^n = 2^(k*n), and k*n >= 256 when n >= 256/k
  // For base divisible by 2, any exponent > 255 gives 0
  if ((base.data_[0] & 1) == 0 && (exponent.data_[0] > 255 || exponent.data_[1] != 0 ||
                                   exponent.data_[2] != 0 || exponent.data_[3] != 0)) {
    return Uint256(0);
  }

  // Binary exponentiation (square-and-multiply)
  // Process exponent as a single multi-word value, shifting the entire number
  Uint256 result(1);
  Uint256 multiplier = base;
  Uint256 exp = exponent;

  while (!exp.is_zero()) {
    if ((exp.data_[0] & 1) != 0) {
      result = result * multiplier;
    }
    multiplier = multiplier * multiplier;
    exp = exp >> 1;
  }

  return result;
}

}  // namespace div0::types
