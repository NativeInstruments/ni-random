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
// clang-format on

#pragma once

//! \file PhiloxMath.hpp
//!
//! Width-portable widening-multiply helper for the Philox counter-based
//! engines. Splits the 2w-bit product of two w-bit operands into its low and
//! high w-bit halves -- the mullo / mulhi values the Philox round function
//! needs. Original ni-random code implemented from the C++26 [rand.eng.philox]
//! specification; no third-party source is copied or included. The 64-bit path
//! mirrors the cross-platform 64x64->128 multiply PCG already uses in this repo
//! (detail/PcgUint128.hpp): __uint128_t on Clang/GCC, _umul128 on MSVC. See
//! plans/add-philox-engines.md.

#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

namespace ni::random::detail {

//! Returns the low w bits of the unsigned product a*b; writes the high w bits
//! to hi. Operands and both results are masked to w bits, so a result_type
//! wider than w (e.g. a 64-bit uint_fast32_t) still reproduces the standard's
//! sequence.
template <int W, class T> constexpr T mulhilo(T a, T b, T &hi) {
  static_assert(std::is_unsigned_v<T>, "mulhilo requires an unsigned type");
  static_assert(W == 32 || W == 64, "Philox supports w = 32 or w = 64");

  constexpr T mask = (W >= static_cast<int>(sizeof(T) * 8))
                         ? static_cast<T>(~T(0))
                         : static_cast<T>((T(1) << W) - 1);

  if constexpr (W == 32) {
    const std::uint64_t product = static_cast<std::uint64_t>(a & mask) *
                                  static_cast<std::uint64_t>(b & mask);
    hi = static_cast<T>((product >> 32) & 0xFFFFFFFFu);
    return static_cast<T>(product & 0xFFFFFFFFu);
  } else {
    const std::uint64_t lhs = static_cast<std::uint64_t>(a & mask);
    const std::uint64_t rhs = static_cast<std::uint64_t>(b & mask);
#if defined(_MSC_VER)
    std::uint64_t high = 0;
    const std::uint64_t low = _umul128(lhs, rhs, &high);
    hi = static_cast<T>(high);
    return static_cast<T>(low);
#elif defined(__SIZEOF_INT128__)
    const __uint128_t product =
        static_cast<__uint128_t>(lhs) * static_cast<__uint128_t>(rhs);
    hi = static_cast<T>(product >> 64);
    return static_cast<T>(product);
#else
#error                                                                         \
    "Philox4x64 requires a 64x64->128 multiply (__uint128_t or MSVC _umul128)"
#endif
  }
}

} // namespace ni::random::detail
