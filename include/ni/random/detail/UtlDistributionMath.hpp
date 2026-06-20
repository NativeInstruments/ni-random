// clang-format off
//
// Copyright (c) 2026 Roth Michaels
// Copyright (c) 2026 Native Instruments USA, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
// Portions of this file derive from third-party software; the original
// copyright and license notice is retained below and continues to apply.
//
// clang-format on

// clang-format off
//----------------------------------------------------------------------------------------
//
//	DmitriBogdanov/UTL — utl::random
//	https://github.com/DmitriBogdanov/UTL
//
//	MIT License
//
//	Copyright (c) 2023 Dmitri Bogdanov
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//----------------------------------------------------------------------------------------
// clang-format on

#pragma once

//! \file UtlDistributionMath.hpp
//!
//! Shared sampling math for the UTL distribution family, imported from
//! DmitriBogdanov/UTL (utl::random, random.hpp). The algorithm bodies are
//! unmodified from upstream; only the enclosing namespace (utl::random[::impl]
//! -> ni::random::detail) and the formatting (LLVM style) differ, and only the
//! subset the four distributions need is imported. See
//! plans/import-utl-distributions.md and provenance random-repos/UTL.
//!
//! This is an internal helper header. generate_canonical() is NOT re-exported
//! as public ni::random API by this task.

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace ni::random::detail {

// ============================
// --- SFINAE & type traits ---
// ============================

template <class> constexpr bool always_false_v = false;

template <bool Cond>
using require =
    std::enable_if_t<Cond, bool>; // makes SFINAE a bit less cumbersome

template <class T> using require_integral = require<std::is_integral_v<T>>;

template <class T> using require_float = require<std::is_floating_point_v<T>>;

template <class T>
using require_uint = require<std::is_integral_v<T> && std::is_unsigned_v<T>>;

// ====================
// --- 128-bit uint ---
// ====================

// We need 128-bit uint for Lemire's uniform integer distribution algorithm.

// GCC & clang provide 128-bit integers as compiler extension
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
using Uint128 = __uint128_t;

// Otherwise fallback onto either MSVC intrinsics or manual emulation
#else

// Emulation of 128-bit unsigned integer tailored specifically for usage in
// 64-bit Lemire's algorithm, this allows us to skip a lot of generic logic
// since we really only need 3 things:
//
//    1) 'uint128(x) * uint128(y)'       that performs 64x64 -> 128 bit
//    multiplication 2) 'static_cast<std::uint64_t>(x)' that returns lower 64
//    bits 3) 'x >> 64'                       that returns upper 64 bits
//
struct Uint128 {
  std::uint64_t low{}, high{};

  constexpr Uint128(std::uint64_t low) noexcept : low(low) {}
  constexpr explicit Uint128(std::uint64_t low, std::uint64_t high) noexcept
      : low(low), high(high) {}

  [[nodiscard]] constexpr operator std::uint64_t() const noexcept {
    return this->low;
  }

  [[nodiscard]] constexpr Uint128 operator*(Uint128 other) const noexcept {
#if defined(UTL_RANDOM_USE_INTRINSICS) && defined(_MSC_VER) &&                 \
    (defined(__x86_64__) || defined(__amd64__))
    // Unlike GCC, MSVC also requires 'UTL_RANDOM_USE_INTRINSICS' flag since it
    // also needs '#include <intrin.h>' for 128-bit multiplication, which could
    // be considered a somewhat intrusive thing to include

    std::uint64_t upper = 0;
    std::uint64_t lower = _umul128(this->low, other.low, &upper);

    return Uint128{lower, upper};

#else

    // Compute all of the cross products
    const std::uint64_t lo_lo =
        (this->low & 0xFFFFFFFF) * (other.low & 0xFFFFFFFF);
    const std::uint64_t hi_lo = (this->low >> 32) * (other.low & 0xFFFFFFFF);
    const std::uint64_t lo_hi = (this->low & 0xFFFFFFFF) * (other.low >> 32);
    const std::uint64_t hi_hi = (this->low >> 32) * (other.low >> 32);

    // Add products together, this will never overflow
    const std::uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    const std::uint64_t upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
    const std::uint64_t lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

    return Uint128{lower, upper};

#endif
  }

  [[nodiscard]] constexpr Uint128 operator>>(int) const noexcept {
    return this->high;
  }
};

#endif

// clang-format off
template<class T> struct wider { static_assert(always_false_v<T>, "Missing specialization."); };

template<> struct wider<std::uint8_t > { using type = std::uint16_t; };
template<> struct wider<std::uint16_t> { using type = std::uint32_t; };
template<> struct wider<std::uint32_t> { using type = std::uint64_t; };
template<> struct wider<std::uint64_t> { using type = Uint128;       };

template<class T> using wider_t = typename wider<T>::type;
// clang-format on

// ===========================
// --- Bit-twiddling utils ---
// ===========================

template <class T, require_uint<T> = true>
[[nodiscard]] constexpr T uint_minus(T value) noexcept {
  return ~value + T(1);
  // MSVC with '/W2' warning level gives a warning when using unary minus with
  // an unsigned value, this warning gets elevated to a compilation error by
  // '/sdl' flag, see
  // https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4146
  //
  // This is a case of MSVC not being standard-compliant, as unsigned '-x' is a
  // perfectly defined operation which evaluates to the same thing as '~x + 1u'.
  // To work around such warning we define this function
}

// ============================
// --- Uniform int sampling ---
// ============================

template <class T, class Gen, require_uint<T> = true>
constexpr T uniform_uint_lemire(Gen &gen, T range) noexcept(noexcept(gen())) {
  using W = wider_t<T>;

  W product = W(gen()) * W(range);
  T low = T(product);
  if (low < range) {
    while (low < uint_minus(range) % range) {
      product = W(gen()) * W(range);
      low = T(product);
    }
  }
  return product >> std::numeric_limits<T>::digits;
}

// Reimplementation of libstdc++ 'std::uniform_int_distribution<>' except
// - constexpr
// - const-qualified (relative to distribution parameters)
// - noexcept as long as 'Gen::operator()' is noexcept, which is true for all
//   generators in this module
// - supports 'std::uint8_t', 'std::int8_t', 'char'
// - produces the same sequence on each platform
// Performance is exactly the same a libstdc++ version of
// 'std::uniform_int_distribution<>', in fact, it is likely to return the exact
// same sequence for most types
template <class T, class Gen, require_integral<T> = true>
constexpr T generate_uniform_int(Gen &gen, T min,
                                 T max) noexcept(noexcept(gen())) {
  using result_type = T;
  using unsigned_type = std::make_unsigned_t<result_type>;
  using generated_type = typename Gen::result_type;
  using common_type = std::common_type_t<unsigned_type, generated_type>;

  constexpr common_type prng_min = Gen::min();
  constexpr common_type prng_max = Gen::max();

  static_assert(prng_min < prng_max,
                "UniformRandomBitGenerator requires 'min() < max()'");

  constexpr common_type prng_range = prng_max - prng_min;
  constexpr common_type type_range = std::numeric_limits<common_type>::max();
  const common_type range = common_type(max) - common_type(min);

  common_type res{};

  // PRNG has enough state for the range
  if (prng_range > range) {
    const common_type ext_range =
        range + 1; // never overflows due to branch condition

    // PRNG is bit-uniform
    // => use Lemire's algorithm, GCC/clang provide 128-bit arithmetics
    //    natively, other compilers use emulation, Lemire with emulated 128-bit
    //    ints performs about the same as Java's "modx1", which is the best
    //    algorithm without wide arithmetics
    if constexpr (prng_range == type_range) {
      res = uniform_uint_lemire<common_type>(gen, ext_range);
    }
    // PRNG is non-uniform (usually because 'prng_min' is '1')
    // => fallback onto a 2-division algorithm
    else {
      const common_type scaling = prng_range / ext_range;
      const common_type past = ext_range * scaling;

      do {
        res = common_type(gen()) - prng_min;
      } while (res >= past);
      res /= scaling;
    }
  }
  // PRNG needs several invocations to acquire enough state for the range
  else if (prng_range < range) {
    common_type temp{};
    do {
      constexpr common_type ext_prng_range =
          (prng_range < type_range) ? prng_range + 1 : type_range;
      temp = ext_prng_range *
             generate_uniform_int<common_type>(gen, 0, range / ext_prng_range);
      res = temp + (common_type(gen()) - prng_min);
    } while (res >= range || res < temp);
  } else {
    res = common_type(gen()) - prng_min;
  }

  return static_cast<T>(min + res);

  // Note 1:
  // 'static_cast<>()' preserves bit pattern of signed/unsigned integers of the
  // same size as long as those integers are two-complement (see
  // https://en.wikipedia.org/wiki/Two's_complement), this is true for most
  // platforms and is in fact guaranteed for standard fixed-width types like
  // 'uint32_t' on any platform (see
  // https://en.cppreference.com/w/cpp/types/integer)
  //
  // This means signed integer distribution can simply use unsigned algorithm
  // and reinterpret the result internally. This would be a bit nicer
  // semantically with C++20 `std::bit_cast<>`, but not ultimately any
  // different.

  // Note 2:
  // 'ext_prng_range' has a ternary purely to silence a false compiler warning
  // about division by zero due to 'prng_range + 1' overflowing into '0' when
  // 'prng_range' is equal to 'type_range'. Falling into this runtime branch
  // requires 'prng_range < range <= type_range' making such situation
  // impossible, here we simply clamp the value to 'type_range' so it doesn't
  // overflow and trip the compiler when analyzing constexpr for potential UB.

  // Note 3:
  // 'static_cast<T>()' in return is functionally useless, but prevents some
  // false positive warnings on MSVC
}

// ============================
// --- Canonical real value ---
// ============================

// Can't really make things work without making some reasonable assumptions
// about the size of primitive types, this should be satisfied for the vast
// majority of platforms. Esoteric architectures can manually adapt the
// algorithm if that is necessary.
static_assert(std::numeric_limits<double>::digits == 53,
              "Platform not supported, 'double' is expected to be 64-bit.");
static_assert(std::numeric_limits<float>::digits == 24,
              "Platform not supported, 'float' is expected to be 32-bit.");

template <class T> constexpr int bit_width(T value) noexcept {
  int width = 0;
  while (value >>= 1)
    ++width;
  return width;
}

// Constexpr reimplementation of 'std::generate_canonical<>()'
template <class T, class Gen>
constexpr T generate_canonical_generic(Gen &gen) noexcept(noexcept(gen())) {
  using float_type = T;
  using generated_type = typename Gen::result_type;

  constexpr int float_bits = std::numeric_limits<float_type>::digits;
  // always produce enough bits of randomness for the whole mantissa

  constexpr generated_type prng_max = Gen::max();
  constexpr generated_type prng_min = Gen::min();

  constexpr generated_type prng_range = prng_max - prng_min;
  constexpr generated_type type_range =
      std::numeric_limits<generated_type>::max();

  constexpr int prng_bits = (prng_range < type_range)
                                ? bit_width(prng_range + 1)
                                : 1 + bit_width(prng_range);
  // how many full bits of randomness PRNG produces on each invocation,
  // prng_bits == floor(log2(prng_range + 1)), ternary handles the case that
  // would overflow when (prng_range == type_range)

  constexpr int invocations_needed = [&]() {
    int count = 0;
    for (int generated_bits = 0; generated_bits < float_bits;
         generated_bits += prng_bits)
      ++count;
    return count;
  }();
  // GCC and MSVC use runtime conversion to floating point and std::ceil() &
  // std::log() to obtain this value, in MSVC (before LWG 2524 implementation)
  // for example we had something like this:
  //    > invocations_needed = std::ceil( float_type(float_bits) / std::log2(
  //    > float_type(prng_range) + 1 ) )
  // which is not constexpr due to math functions, we can do a similar thing
  // much easier by just counting bits generated per each invocation. This
  // returns the same thing for any sane PRNG, except since it only counts "full
  // bits" esoteric ranges such as [1, 3] which technically have 1.5 bits of
  // randomness will be counted as 1 bit of randomness, thus overestimating the
  // invocations a little. In practice this makes 0 difference since its only
  // matters for exceedingly small 'prng_range' and such PRNGs simply don't
  // exist in nature, and even if they are theoretically used they will simply
  // use a few more invocations to produce a proper result

  constexpr float_type prng_float_max = static_cast<float_type>(prng_max);
  constexpr float_type prng_float_min = static_cast<float_type>(prng_min);
  constexpr float_type prng_float_range =
      (prng_float_max - prng_float_min) + float_type(1);

  float_type res = float_type(0);
  float_type factor = float_type(1);

  for (int i = 0; i < invocations_needed; ++i) {
    res +=
        (static_cast<float_type>(gen()) - static_cast<float_type>(prng_min)) *
        factor;
    factor *= prng_float_range;
  }
  res /= factor;
  // same algorithm is used by 'std::generate_canonical<>' in GCC/clang as of
  // 2025, MSVC used to do the same before the P0952R2 overhaul (see
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0952r2.html)

  if (res >= float_type(1))
    res = float_type(1) -
          std::numeric_limits<float_type>::epsilon() / float_type(2);
  // GCC patch that fixes occasional generation of '1's, has non-zero effect on
  // performance

  return res;
}

// Wrapper that adds special case optimizations for
// `generate_canonical_generic<>()'
template <class T, class Gen>
constexpr T generate_canonical(Gen &gen) noexcept(noexcept(gen())) {
  using float_type = T;
  using generated_type = typename Gen::result_type;

  constexpr generated_type prng_min = Gen::min();
  constexpr generated_type prng_max = Gen::max();

  static_assert(prng_min < prng_max,
                "UniformRandomBitGenerator requires 'min() < max()'");

  constexpr generated_type prng_range = prng_max - prng_min;
  constexpr generated_type type_range =
      std::numeric_limits<generated_type>::max();
  constexpr bool prng_is_bit_uniform = (prng_range == type_range);

  constexpr int exponent_bits_64 = 11;
  constexpr int exponent_bits_32 = 8;

  constexpr double mantissa_hex_64 =
      0x1.0p-53; // == 2^-53, corresponds to 53 significant bits of double
  constexpr float mantissa_hex_32 =
      0x1.0p-24f; // == 2^-24, corresponds to 24 significant bits of float

  constexpr double pow2_minus_64 = 0x1.0p-64; // == 2^-64
  constexpr double pow2_minus_32 = 0x1.0p-32; // == 2^-32

  // Note 1: Note hexadecimal float literals, 'p' separates hex-base from the
  // exponent Note 2: Floats have 'mantissa_size + 1' significant bits due to
  // having a sign bit Note 3: All of the methods below produce [0, 1) range

  // Bit-uniform PRNGs can be simply bitmasked & shifted to obtain mantissa
  // 64-bit float, 64-bit uniform PRNG
  // => multiplication algorithm, see https://prng.di.unimi.it/
  if constexpr (prng_is_bit_uniform && sizeof(float_type) == 8 &&
                sizeof(generated_type) == 8) {
    return (gen() >> exponent_bits_64) * mantissa_hex_64;
  }
  // 64-bit float, 32-bit uniform PRNG
  // => "low-high" algorithm, see
  // https://www.doornik.com/research/randomdouble.pdf
  else if constexpr (prng_is_bit_uniform && sizeof(T) == 8 &&
                     sizeof(generated_type) == 4) {
    return (gen() * pow2_minus_64) + (gen() * pow2_minus_32);
  }
  // 32-bit float, 64-bit uniform PRNG
  // => discard bits + multiplication algorithm
  else if constexpr (prng_is_bit_uniform && sizeof(T) == 4 &&
                     sizeof(generated_type) == 8) {
    return (static_cast<std::uint32_t>(gen()) >> exponent_bits_32) *
           mantissa_hex_32;
  }
  // 32-bit float, 32-bit uniform PRNG
  // => multiplication algorithm tweaked for 32-bit
  else if constexpr (prng_is_bit_uniform && sizeof(T) == 4 &&
                     sizeof(generated_type) == 4) {
    return (gen() >> exponent_bits_32) * mantissa_hex_32;
  }
  // Generic case, no particular optimizations can be made
  else {
    return generate_canonical_generic<T>(gen);
  }
}

// =================================
// --- Approximate normal value ---
// =================================

template <class T, require_uint<T> = true>
[[nodiscard]] constexpr int popcount(T x) noexcept {
  constexpr auto bitmask_1 = static_cast<T>(0x5555555555555555UL);
  constexpr auto bitmask_2 = static_cast<T>(0x3333333333333333UL);
  constexpr auto bitmask_3 = static_cast<T>(0x0F0F0F0F0F0F0F0FUL);

  constexpr auto bitmask_16 = static_cast<T>(0x00FF00FF00FF00FFUL);
  constexpr auto bitmask_32 = static_cast<T>(0x0000FFFF0000FFFFUL);
  constexpr auto bitmask_64 = static_cast<T>(0x00000000FFFFFFFFUL);

  x = (x & bitmask_1) + ((x >> 1) & bitmask_1);
  x = (x & bitmask_2) + ((x >> 2) & bitmask_2);
  x = (x & bitmask_3) + ((x >> 4) & bitmask_3);

  if constexpr (sizeof(T) > 1)
    x = (x & bitmask_16) + ((x >> 8) & bitmask_16);
  if constexpr (sizeof(T) > 2)
    x = (x & bitmask_32) + ((x >> 16) & bitmask_32);
  if constexpr (sizeof(T) > 4)
    x = (x & bitmask_64) + ((x >> 32) & bitmask_64);

  return x; // GCC seem to be smart enough to replace this with a built-in
} // C++20 adds a proper 'std::popcount()'

// Quick approximation of normal distribution based on this excellent reddit
// thread:
// https://www.reddit.com/r/algorithms/comments/yyz59u/fast_approximate_gaussian_generator/
//
// Lack of <cmath> functions also allows us to 'constexpr' everything

template <class T>
[[nodiscard]] constexpr T
approx_standard_normal_from_u32_pair(std::uint32_t major,
                                     std::uint32_t minor) noexcept {
  constexpr T delta = T(1) / T(4294967296); // (1 / 2^32)

  T x = T(popcount(major)); // random binomially distributed integer 0 to 32
  x += minor * delta;       // linearly fill the gaps between integers
  x -= T(16.5);             // re-center around 0 (the mean should be 16+0.5)
  x *= T(0.3535534);        // scale to ~1 standard deviation
  return x;

  // 'x' now has a mean of 0, stddev very close to 1, and lies strictly in
  // [-5.833631, 5.833631] range, there are exactly 33 * 2^32 possible outputs
  // which is slightly more than 37 bits of entropy, the distribution is
  // approximated via 33 equally spaced intervals each of which is further
  // subdivided into 2^32 parts. As a result we have a very fast, but noticeably
  // inaccurate approximation, not suitable for research, but might prove very
  // useful in fuzzing / gamedev where quality is not that important.
}

template <class T>
[[nodiscard]] constexpr T
approx_standard_normal_from_u64(std::uint64_t rng) noexcept {
  return approx_standard_normal_from_u32_pair<T>(
      static_cast<std::uint32_t>(rng >> 32), static_cast<std::uint32_t>(rng));
}

template <class T, class Gen>
constexpr T approx_standard_normal(Gen &gen) noexcept {
  // Ensure PRNG is bit-uniform
  using generated_type = typename Gen::result_type;

  static_assert(Gen::min() == 0);
  static_assert(Gen::max() == std::numeric_limits<generated_type>::max());

  // Forward PRNG to a fast approximation
  if constexpr (sizeof(generated_type) == 8) {
    return approx_standard_normal_from_u64<T>(gen());
  } else if constexpr (sizeof(generated_type) == 4) {
    return approx_standard_normal_from_u32_pair<T>(gen(), gen());
  } else {
    static_assert(always_false_v<T>, "ApproxNormalDistribution<> only supports "
                                     "bit-uniform 32/64-bit PRNGs.");
    // we could use a slower fallback for esoteric PRNGs, but I think it's
    // better to explicitly state when "fast approximate" is not available,
    // esoteric PRNGs are already handled by a regular NormalDistribution
  }
}

} // namespace ni::random::detail
