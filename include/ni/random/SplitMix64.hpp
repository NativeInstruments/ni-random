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

//! \file SplitMix64.hpp
//!
//! Verbatim import of the SplitMix64 generator from Xoshiro-cpp
//! (XoshiroCpp.hpp), split into a per-generator header for ni-random. The
//! algorithm body is unmodified; only the enclosing namespace (XoshiroCpp ->
//! ni::random) and the file organization differ from upstream. SplitMix64 is
//! also the seeder for the xoshiro/xoroshiro generators.
//! See plans/import-xoshiro-family.md.

#pragma once
#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/XoshiroDetail.hpp"
#include <array>
#include <cstdint>

namespace ni::random {
// SplitMix64
// Output: 64 bits
// Period: 2^64
// Footprint: 8 bytes
// Original implementation: http://prng.di.unimi.it/splitmix64.c
class SplitMix64 {
public:
  using state_type = std::uint64_t;
  using result_type = std::uint64_t;

  XOSHIROCPP_NODISCARD_CXX20
  explicit constexpr SplitMix64(state_type state = DefaultSeed) noexcept;

  template <class SeedSeq>
    requires detail::SeedSequenceLike<SeedSeq>
  XOSHIROCPP_NODISCARD_CXX20 explicit SplitMix64(SeedSeq &seq) {
    seed(seq);
  }

  constexpr void seed(state_type state = DefaultSeed) noexcept {
    m_state = state;
  }

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

  template <std::size_t N>
  [[nodiscard]] constexpr std::array<std::uint64_t, N>
  generateSeedSequence() noexcept;

  [[nodiscard]] static constexpr result_type min() noexcept;

  [[nodiscard]] static constexpr result_type max() noexcept;

  [[nodiscard]] constexpr state_type serialize() const noexcept;

  constexpr void deserialize(state_type state) noexcept;

  [[nodiscard]] friend bool operator==(const SplitMix64 &lhs,
                                       const SplitMix64 &rhs) noexcept {
    return (lhs.m_state == rhs.m_state);
  }

  [[nodiscard]] friend bool operator!=(const SplitMix64 &lhs,
                                       const SplitMix64 &rhs) noexcept {
    return (lhs.m_state != rhs.m_state);
  }

private:
  state_type m_state;
};

////////////////////////////////////////////////////////////////
//
//	SplitMix64
//
inline constexpr SplitMix64::SplitMix64(const state_type state) noexcept
    : m_state(state) {}

inline constexpr SplitMix64::result_type SplitMix64::operator()() noexcept {
  std::uint64_t z = (m_state += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

template <std::size_t N>
inline constexpr std::array<std::uint64_t, N>
SplitMix64::generateSeedSequence() noexcept {
  std::array<std::uint64_t, N> seeds = {};

  for (auto &seed : seeds) {
    seed = operator()();
  }

  return seeds;
}

inline constexpr SplitMix64::result_type SplitMix64::min() noexcept {
  return std::numeric_limits<result_type>::lowest();
}

inline constexpr SplitMix64::result_type SplitMix64::max() noexcept {
  return std::numeric_limits<result_type>::max();
}

inline constexpr SplitMix64::state_type SplitMix64::serialize() const noexcept {
  return m_state;
}

inline constexpr void SplitMix64::deserialize(const state_type state) noexcept {
  m_state = state;
}

static_assert(RandomNumberEngine<SplitMix64>);
} // namespace ni::random
