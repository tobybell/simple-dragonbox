#pragma once

#include "dragonbox.h"

namespace jkj {
    namespace dragonbox {

template <class Float>
struct ToCharsImpl {
  using format = float_format<Float>;
  using carrier_uint = typename format::carrier_uint;

  template <unsigned max_digits>
  static constexpr char* print_integer_naive(carrier_uint n, char* buffer) noexcept {
      char temp[max_digits]{};
      auto ptr = temp + sizeof(temp) - 1;
      do {
          *ptr = char('0' + n % 10);
          n /= 10;
          --ptr;
      } while (n != 0);
      while (++ptr != temp + sizeof(temp)) {
          *buffer = *ptr;
          ++buffer;
      }
      return buffer;
  }

  static constexpr char* to_chars_naive(carrier_uint significand, int exponent,
                                       char* buffer) noexcept {
      // Print significand.
      {
          auto ptr = print_integer_naive<format::decimal_significand_digits>(significand,
                                                                                  buffer);

          // Insert decimal dot.
          if (ptr > buffer + 1) {
              auto next = *++buffer;
              ++exponent;
              *buffer = '.';
              while (++buffer != ptr) {
                  auto const temp = *buffer;
                  *buffer = next;
                  next = temp;
                  ++exponent;
              }
              *buffer = next;
          }
          ++buffer;
      }

      // Print exponent.
      *buffer = 'E';
      ++buffer;
      if (exponent < 0) {
          *buffer = '-';
          ++buffer;
          exponent = -exponent;
      }
      return print_integer_naive<format::decimal_exponent_digits>(unsigned(exponent),
                                                                       buffer);
  }

  template <class... Policies>
  constexpr static char* to_chars(Float x, char* buffer) noexcept {
    auto br = float_bits(x);
    auto const [significand, exponent, sign] = br;

    if (br.is_finite()) {
        if (sign) {
            *buffer = '-';
            ++buffer;
        }
        if (significand || exponent) {
          auto decimal = to_decimal_ex<Float, Policies...>(sign, exponent, significand);
          return to_chars_naive(decimal.significand, decimal.exponent, buffer);
        }
        else {
            buffer[0] = '0';
            buffer[1] = 'E';
            buffer[2] = '0';
            return buffer + 3;
        }
    }
    else {
        if (!significand) {
            if (sign) {
                *buffer = '-';
                ++buffer;
            }
            std::memcpy(buffer, "Infinity", 8);
            return buffer + 8;
        }
        else {
            buffer[0] = 'N';
            buffer[1] = 'a';
            buffer[2] = 'N';
            return buffer + 3;
        }
    }
  }
};

        // Null-terminate and bypass the return value of fp_to_chars_n
        template <class Float, class... Policies>
        constexpr char* to_chars(Float x, char* buffer, Policies... policies) noexcept {
            auto ptr = ToCharsImpl<Float>::template to_chars<Policies...>(x, buffer);
            *ptr = '\0';
            return ptr;
        }

        // Maximum required buffer size (excluding null-terminator)
        template <class FloatFormat>
        inline constexpr size_t max_output_string_length =
            // sign(1) + significand + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp
            1 + FloatFormat::decimal_significand_digits + 1 + 1 + 1 +
            FloatFormat::decimal_exponent_digits;
    }
}
