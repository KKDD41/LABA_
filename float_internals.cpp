#include "float_internals.h"
#include <iostream>
#include <algorithm>
#include <bitset>
#include <string>

std::string FloatInternalsAsString(float value) {
  auto* int_ptr_to_value = reinterpret_cast<unsigned int*>(&value);

  std::string result;
  result = ((std::bitset<1>(*int_ptr_to_value >> 31) == 1) ? "-" : "+");
  uint8_t exponent = std::bitset<8>(*int_ptr_to_value >> 23).to_ulong();
  std::string mantissa_string = std::bitset<23>(*int_ptr_to_value).to_string();
  if (exponent == 0) {
    if (mantissa_string == "00000000000000000000000") {
      result += "0";
      return result;
    }
    result += "0.";
    exponent++;
  } else if (exponent == 255) {
    result += "INF";
    return result;
  } else {
    result += "1.";
  }
  result +=
      (mantissa_string + "_2 * 2^{" + std::to_string(exponent - 127) + "}");
  return result;
}

