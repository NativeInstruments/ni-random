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

//! \file Xoshiro256StarStar.hpp
//!
//! Verbatim import of the xoshiro256** generator from Xoshiro-cpp
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
// xoshiro256**
// Output: 64 bits
// Period: 2^256 - 1
// Footprint: 32 bytes
// Original implementation: http://prng.di.unimi.it/xoshiro256starstar.c
// Version: 1.0
class Xoshiro256StarStar {
public:
  using state_type = std::array<std::uint64_t, 4>;
  using result_type = std::uint64_t;

  XOSHIROCPP_NODISCARD_CXX20
  explicit constexpr Xoshiro256StarStar(
      std::uint64_t seed = DefaultSeed) noexcept;

  XOSHIROCPP_NODISCARD_CXX20
  explicit constexpr Xoshiro256StarStar(state_type state) noexcept;

  template <class SeedSeq>
    requires detail::SeedSequenceLike<SeedSeq>
  XOSHIROCPP_NODISCARD_CXX20 explicit Xoshiro256StarStar(SeedSeq &seq) {
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
  // to 2^128 calls to next(); it can be used to generate 2^128
  // non-overlapping subsequences for parallel computations.
  constexpr void jump() noexcept;

  // This is the long-jump function for the generator. It is equivalent to
  // 2^192 calls to next(); it can be used to generate 2^64 starting points,
  // from each of which jump() will generate 2^64 non-overlapping
  // subsequences for parallel distributed computations.
  constexpr void longJump() noexcept;

  [[nodiscard]] static constexpr result_type min() noexcept;

  [[nodiscard]] static constexpr result_type max() noexcept;

  [[nodiscard]] constexpr state_type serialize() const noexcept;

  constexpr void deserialize(state_type state) noexcept;

  [[nodiscard]] friend bool operator==(const Xoshiro256StarStar &lhs,
                                       const Xoshiro256StarStar &rhs) noexcept {
    return (lhs.m_state == rhs.m_state);
  }

  [[nodiscard]] friend bool operator!=(const Xoshiro256StarStar &lhs,
                                       const Xoshiro256StarStar &rhs) noexcept {
    return (lhs.m_state != rhs.m_state);
  }

private:
  state_type m_state;
};

////////////////////////////////////////////////////////////////
//
//	xoshiro256**
//
inline constexpr Xoshiro256StarStar::Xoshiro256StarStar(
    const std::uint64_t seedValue) noexcept
    : m_state() {
  seed(seedValue);
}

inline constexpr void
Xoshiro256StarStar::seed(const std::uint64_t seedValue) noexcept {
  m_state = SplitMix64{seedValue}.generateSeedSequence<4>();
}

inline constexpr Xoshiro256StarStar::Xoshiro256StarStar(
    const state_type state) noexcept
    : m_state(state) {}

inline constexpr Xoshiro256StarStar::result_type
Xoshiro256StarStar::operator()() noexcept {
  const std::uint64_t result = detail::RotL(m_state[1] * 5, 7) * 9;
  const std::uint64_t t = m_state[1] << 17;
  m_state[2] ^= m_state[0];
  m_state[3] ^= m_state[1];
  m_state[1] ^= m_state[2];
  m_state[0] ^= m_state[3];
  m_state[2] ^= t;
  m_state[3] = detail::RotL(m_state[3], 45);
  return result;
}

inline constexpr void Xoshiro256StarStar::jump() noexcept {
  constexpr std::uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
                                    0xa9582618e03fc9aa, 0x39abdc4529b1661c};

  std::uint64_t s0 = 0;
  std::uint64_t s1 = 0;
  std::uint64_t s2 = 0;
  std::uint64_t s3 = 0;

  for (std::uint64_t jump : JUMP) {
    for (int b = 0; b < 64; ++b) {
      if (jump & UINT64_C(1) << b) {
        s0 ^= m_state[0];
        s1 ^= m_state[1];
        s2 ^= m_state[2];
        s3 ^= m_state[3];
      }
      operator()();
    }
  }

  m_state[0] = s0;
  m_state[1] = s1;
  m_state[2] = s2;
  m_state[3] = s3;
}

inline constexpr void Xoshiro256StarStar::longJump() noexcept {
  constexpr std::uint64_t LONG_JUMP[] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3,
                                         0x77710069854ee241,
                                         0x39109bb02acbe635};

  std::uint64_t s0 = 0;
  std::uint64_t s1 = 0;
  std::uint64_t s2 = 0;
  std::uint64_t s3 = 0;

  for (std::uint64_t jump : LONG_JUMP) {
    for (int b = 0; b < 64; ++b) {
      if (jump & UINT64_C(1) << b) {
        s0 ^= m_state[0];
        s1 ^= m_state[1];
        s2 ^= m_state[2];
        s3 ^= m_state[3];
      }
      operator()();
    }
  }

  m_state[0] = s0;
  m_state[1] = s1;
  m_state[2] = s2;
  m_state[3] = s3;
}

inline constexpr Xoshiro256StarStar::result_type
Xoshiro256StarStar::min() noexcept {
  return std::numeric_limits<result_type>::lowest();
}

inline constexpr Xoshiro256StarStar::result_type
Xoshiro256StarStar::max() noexcept {
  return std::numeric_limits<result_type>::max();
}

inline constexpr Xoshiro256StarStar::state_type
Xoshiro256StarStar::serialize() const noexcept {
  return m_state;
}

inline constexpr void
Xoshiro256StarStar::deserialize(const state_type state) noexcept {
  m_state = state;
}

static_assert(RandomNumberEngine<Xoshiro256StarStar>);
} // namespace ni::random
