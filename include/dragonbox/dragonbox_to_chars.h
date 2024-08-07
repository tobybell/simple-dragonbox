#pragma once

#include "dragonbox.h"

namespace jkj {
    namespace dragonbox {
        namespace detail {
            template <stdr::size_t max_digits, class UInt>
            constexpr char* print_integer_naive(UInt n, char* buffer) noexcept {
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

            template <class FloatFormat, class CarrierUInt>
            constexpr char* to_chars_naive(CarrierUInt significand, int exponent,
                                                 char* buffer) noexcept {
                // Print significand.
                {
                    auto ptr = print_integer_naive<FloatFormat::decimal_significand_digits>(significand,
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
                return print_integer_naive<FloatFormat::decimal_exponent_digits>(unsigned(exponent),
                                                                                 buffer);
            }
        }


template <class Float>
struct ToCharsImpl {
  using FormatTraits = ieee754_binary_traits<Float>;

  template <class DecimalToBinaryRoundingPolicy, class BinaryToDecimalRoundingPolicy,
            class CachePolicy, class FormatTraits>
  static constexpr char*
  compact_to_chars(bool sign,
           typename FormatTraits::exponent_int exponent,
           typename FormatTraits::carrier_uint significand, char* buffer) noexcept {
      auto result = to_decimal_ex<Float, policy::sign::ignore_t, policy::trailing_zero::remove_compact_t, DecimalToBinaryRoundingPolicy, BinaryToDecimalRoundingPolicy, CachePolicy>(sign, exponent, significand);

      return detail::to_chars_naive<typename FormatTraits::format>(
          result.significand, result.exponent, buffer);
  }

  // Avoid needless ABI overhead incurred by tag dispatch.
  template <class DecimalToBinaryRoundingPolicy, class BinaryToDecimalRoundingPolicy,
            class CachePolicy>
  constexpr static char* to_chars_n_impl(float_bits<FormatTraits> br, char* buffer) noexcept {
      auto const exponent_bits = br.extract_exponent_bits();
      auto const s = br.remove_exponent_bits();
      bool sign = s.is_negative();

      if (br.is_finite(exponent_bits)) {
          if (s.is_negative()) {
              *buffer = '-';
              ++buffer;
          }
          if (br.is_nonzero()) {
            return compact_to_chars<DecimalToBinaryRoundingPolicy,
              BinaryToDecimalRoundingPolicy, CachePolicy, FormatTraits>(sign, exponent_bits, s.significand(), buffer);
          }
          else {
              buffer[0] = '0';
              buffer[1] = 'E';
              buffer[2] = '0';
              return buffer + 3;
          }
      }
      else {
          if (s.has_all_zero_significand_bits()) {
              if (s.is_negative()) {
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

  template <class... Policies>
  constexpr static char* to_chars_n(Float x, char* buffer, Policies...) noexcept {
    using policy_holder = detail::make_policy_holder<
        detail::detector_default_pair_list<
            detail::detector_default_pair<
                detail::is_decimal_to_binary_rounding_policy,
                policy::decimal_to_binary_rounding::nearest_to_even_t>,
            detail::detector_default_pair<detail::is_binary_to_decimal_rounding_policy,
                                          policy::binary_to_decimal_rounding::to_even_t>,
            detail::detector_default_pair<detail::is_cache_policy, policy::cache::full_t>>,
        Policies...>;

    return to_chars_n_impl<typename policy_holder::decimal_to_binary_rounding_policy,
                           typename policy_holder::binary_to_decimal_rounding_policy,
                           typename policy_holder::cache_policy>(
        detail::impl<Float>::make_float_bits(x), buffer);
  }
};

        // Null-terminate and bypass the return value of fp_to_chars_n
        template <class Float, class... Policies>
        constexpr char* to_chars(Float x, char* buffer, Policies... policies) noexcept {
            auto ptr = ToCharsImpl<Float>::to_chars_n(x, buffer, policies...);
            *ptr = '\0';
            return ptr;
        }

        // Maximum required buffer size (excluding null-terminator)
        template <class FloatFormat>
        inline constexpr detail::stdr::size_t max_output_string_length =
            // sign(1) + significand + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp
            1 + FloatFormat::decimal_significand_digits + 1 + 1 + 1 +
            FloatFormat::decimal_exponent_digits;
    }
}
