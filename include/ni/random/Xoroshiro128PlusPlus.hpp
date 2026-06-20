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
//	Xoshiro-cpp
//	Xoshiro PRNG wrapper library for C++17 / C++20
//
//	Copyright (C) 2020 Ryo Suzuki <reputeless@gmail.com>
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------
// clang-format on

//! \file Xoroshiro128PlusPlus.hpp
//!
//! Verbatim import of the xoroshiro128++ generator from Xoshiro-cpp
//! (XoshiroCpp.hpp), split into a per-generator header for ni-random. The
//! algorithm body is unmodified; only the enclosing namespace (XoshiroCpp ->
//! ni::random) and the file organization differ from upstream.
//! See plans/import-xoshiro-family.md.

#pragma once
#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/SplitMix64.hpp"
#include "ni/random/detail/XoshiroDetail.hpp"
#include <array>
#include <cstdint>

namespace ni::random {
// xoroshiro128++
// Output: 64 bits
// Period: 2^128 - 1
// Footprint: 16 bytes
// Original implementation: http://prng.di.unimi.it/xoroshiro128plusplus.c
// Version: 1.0
class Xoroshiro128PlusPlus {
public:
  using state_type = std::array<std::uint64_t, 2>;
  using result_type = std::uint64_t;

  XOSHIROCPP_NODISCARD_CXX20
  explicit constexpr Xoroshiro128PlusPlus(
      std::uint64_t seed = DefaultSeed) noexcept;

  XOSHIROCPP_NODISCARD_CXX20
  explicit constexpr Xoroshiro128PlusPlus(state_type state) noexcept;

  template <class SeedSeq>
    requires detail::SeedSequenceLike<SeedSeq>
  XOSHIROCPP_NODISCARD_CXX20 explicit Xoroshiro128PlusPlus(SeedSeq &seq) {
    seed(seq);
  }

  constexpr void seed(std::uint64_t seedValue = DefaultSeed) noexcept;

  template <class SeedSeq>
    requires detail::SeedSequenceLike<SeedSeq>
  void seed(SeedSeq &seq) {
    detail::SeedSeqFill(seq, m_state);
  }

  constexpr void discard(unsigned long long z) noexcept {
    for (unsigned long long i = 0; i < z; ++i) {
      operator()();
    }
  }

  constexpr result_type operator()() noexcept;

  // This is the jump function for the generator. It is equivalent
  // to 2^64 calls to next(); it can be used to generate 2^64
  // non-overlapping subsequences for parallel computations.
  constexpr void jump() noexcept;

  // This is the long-jump function for the generator. It is equivalent to
  // 2^96 calls to next(); it can be used to generate 2^32 starting points,
  // from each of which jump() will generate 2^32 non-overlapping
  // subsequences for parallel distributed computations.
  constexpr void longJump() noexcept;

  [[nodiscard]] static constexpr result_type min() noexcept;

  [[nodiscard]] static constexpr result_type max() noexcept;

  [[nodiscard]] constexpr state_type serialize() const noexcept;

  constexpr void deserialize(state_type state) noexcept;

  [[nodiscard]] friend bool
  operator==(const Xoroshiro128PlusPlus &lhs,
             const Xoroshiro128PlusPlus &rhs) noexcept {
    return (lhs.m_state == rhs.m_state);
  }

  [[nodiscard]] friend bool
  operator!=(const Xoroshiro128PlusPlus &lhs,
             const Xoroshiro128PlusPlus &rhs) noexcept {
    return (lhs.m_state != rhs.m_state);
  }

private:
  state_type m_state;
};

////////////////////////////////////////////////////////////////
//
//	xoroshiro128++
//
inline constexpr Xoroshiro128PlusPlus::Xoroshiro128PlusPlus(
    const std::uint64_t seedValue) noexcept
    : m_state() {
  seed(seedValue);
}

inline constexpr void
Xoroshiro128PlusPlus::seed(const std::uint64_t seedValue) noexcept {
  m_state = SplitMix64{seedValue}.generateSeedSequence<2>();
}

inline constexpr Xoroshiro128PlusPlus::Xoroshiro128PlusPlus(
    const state_type state) noexcept
    : m_state(state) {}

inline constexpr Xoroshiro128PlusPlus::result_type
Xoroshiro128PlusPlus::operator()() noexcept {
  const std::uint64_t s0 = m_state[0];
  std::uint64_t s1 = m_state[1];
  const std::uint64_t result = detail::RotL(s0 + s1, 17) + s0;
  s1 ^= s0;
  m_state[0] = detail::RotL(s0, 49) ^ s1 ^ (s1 << 21);
  m_state[1] = detail::RotL(s1, 28);
  return result;
}

inline constexpr void Xoroshiro128PlusPlus::jump() noexcept {
  constexpr std::uint64_t JUMP[] = {0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05};

  std::uint64_t s0 = 0;
  std::uint64_t s1 = 0;

  for (std::uint64_t jump : JUMP) {
    for (int b = 0; b < 64; ++b) {
      if (jump & UINT64_C(1) << b) {
        s0 ^= m_state[0];
        s1 ^= m_state[1];
      }
      operator()();
    }
  }

  m_state[0] = s0;
  m_state[1] = s1;
}

inline constexpr void Xoroshiro128PlusPlus::longJump() noexcept {
  constexpr std::uint64_t LONG_JUMP[] = {0x360fd5f2cf8d5d99,
                                         0x9c6e6877736c46e3};

  std::uint64_t s0 = 0;
  std::uint64_t s1 = 0;

  for (std::uint64_t jump : LONG_JUMP) {
    for (int b = 0; b < 64; ++b) {
      if (jump & UINT64_C(1) << b) {
        s0 ^= m_state[0];
        s1 ^= m_state[1];
      }
      operator()();
    }
  }

  m_state[0] = s0;
  m_state[1] = s1;
}

inline constexpr Xoroshiro128PlusPlus::result_type
Xoroshiro128PlusPlus::min() noexcept {
  return std::numeric_limits<result_type>::lowest();
}

inline constexpr Xoroshiro128PlusPlus::result_type
Xoroshiro128PlusPlus::max() noexcept {
  return std::numeric_limits<result_type>::max();
}

inline constexpr Xoroshiro128PlusPlus::state_type
Xoroshiro128PlusPlus::serialize() const noexcept {
  return m_state;
}

inline constexpr void
Xoroshiro128PlusPlus::deserialize(const state_type state) noexcept {
  m_state = state;
}

static_assert(RandomNumberEngine<Xoroshiro128PlusPlus>);
} // namespace ni::random
