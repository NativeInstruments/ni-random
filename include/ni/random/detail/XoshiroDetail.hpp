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

//! \file XoshiroDetail.hpp
//!
//! Shared helpers for the xoshiro/xoroshiro family, imported verbatim from
//! Xoshiro-cpp (XoshiroCpp.hpp). Holds the default seed value, the
//! bits->floating-point converters, and the rotate-left primitive used by
//! every generator. The bodies are unmodified from upstream; only the
//! enclosing namespace (XoshiroCpp -> ni::random) and the file organization
//! (one generator per header) differ. See plans/import-xoshiro-family.md.

#pragma once
#include <array>
#include <concepts>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>
#if __has_cpp_attribute(nodiscard) >= 201907L
#define XOSHIROCPP_NODISCARD_CXX20 [[nodiscard]]
#else
#define XOSHIROCPP_NODISCARD_CXX20
#endif

namespace ni::random {
// A default seed value for the generators
inline constexpr std::uint64_t DefaultSeed = 1234567890ULL;

// Converts given uint32 value `i` into a 32-bit floating
// point value in the range of [0.0f, 1.0f)
template <class Uint32,
          std::enable_if_t<std::is_same_v<Uint32, std::uint32_t>> * = nullptr>
[[nodiscard]] inline constexpr float FloatFromBits(Uint32 i) noexcept;

// Converts given uint64 value `i` into a 64-bit floating
// point value in the range of [0.0, 1.0)
template <class Uint64,
          std::enable_if_t<std::is_same_v<Uint64, std::uint64_t>> * = nullptr>
[[nodiscard]] inline constexpr double DoubleFromBits(Uint64 i) noexcept;

template <class Uint32,
          std::enable_if_t<std::is_same_v<Uint32, std::uint32_t>> *>
inline constexpr float FloatFromBits(const Uint32 i) noexcept {
  return (i >> 8) * 0x1.0p-24f;
}

template <class Uint64,
          std::enable_if_t<std::is_same_v<Uint64, std::uint64_t>> *>
inline constexpr double DoubleFromBits(const Uint64 i) noexcept {
  return (i >> 11) * 0x1.0p-53;
}

namespace detail {
[[nodiscard]] static constexpr std::uint64_t RotL(const std::uint64_t x,
                                                  const int s) noexcept {
  return (x << s) | (x >> (64 - s));
}

[[nodiscard]] static constexpr std::uint32_t RotL(const std::uint32_t x,
                                                  const int s) noexcept {
  return (x << s) | (x >> (32 - s));
}

// The seed-sequence fill below (32->64-bit pairwise merge and the zero-state
// remap) follows the pattern in DmitriBogdanov/UTL (utl::random), MIT-licensed:
// clang-format off
//----------------------------------------------------------------------------------------
//	DmitriBogdanov/UTL — utl::random
//	Copyright (c) 2023 Dmitri Bogdanov
//	Licensed under the MIT License (notice required in all copies). See LICENSE.md
//	in https://github.com/DmitriBogdanov/UTL. Provided "AS IS", without warranty.
//----------------------------------------------------------------------------------------
// clang-format on

// True when every element of the state array is zero. The xoshiro/xoroshiro
// generators have an all-zero fixed point, so a seed sequence that fills the
// state with zeros must be remapped (SeedSeqFill below) before use.
template <class T, std::size_t N>
[[nodiscard]] constexpr bool IsZeroState(const std::array<T, N> &state) {
  for (const auto &e : state) {
    if (e != T(0)) {
      return false;
    }
  }
  return true;
}

// Fill a 64-bit scalar state from a seed sequence. std::seed_seq yields 32-bit
// words, so two are merged into the 64-bit state.
template <class SeedSeq> void SeedSeqFill(SeedSeq &seq, std::uint64_t &dest) {
  std::array<std::uint32_t, 2> temp = {};
  seq.generate(temp.begin(), temp.end());
  dest = static_cast<std::uint64_t>(temp[0]) |
         (static_cast<std::uint64_t>(temp[1]) << 32);
}

// Fill a 32-bit-element state array directly from the seed sequence, remapping
// an all-zero result away from the generators' fixed point.
template <class SeedSeq, std::size_t N>
void SeedSeqFill(SeedSeq &seq, std::array<std::uint32_t, N> &dest) {
  seq.generate(dest.begin(), dest.end());
  if (IsZeroState(dest)) {
    dest[0] = 0x9E3779B9u;
  }
}

// Fill a 64-bit-element state array: generate twice as many 32-bit words and
// merge each pair, remapping an all-zero result.
template <class SeedSeq, std::size_t N>
void SeedSeqFill(SeedSeq &seq, std::array<std::uint64_t, N> &dest) {
  std::array<std::uint32_t, N * 2> temp = {};
  seq.generate(temp.begin(), temp.end());
  for (std::size_t i = 0; i < N; ++i) {
    dest[i] = static_cast<std::uint64_t>(temp[2 * i]) |
              (static_cast<std::uint64_t>(temp[2 * i + 1]) << 32);
  }
  if (IsZeroState(dest)) {
    dest[0] = 0x9E3779B97F4A7C15ull;
  }
}

// A minimal seed-sequence witness: anything exposing generate(first, last).
// Used to constrain the family's seed-sequence ctor/seed() so they never
// hijack the value or copy constructors.
template <class S>
concept SeedSequenceLike =
    requires(S &s, std::uint32_t *p) { s.generate(p, p); };

// An engine whose state can be round-tripped via serialize()/deserialize().
// Constrains the generic stream operators below so they apply only to the
// xoshiro/xoroshiro family.
template <class E>
concept XoshiroSerializable = requires(E &e, const E &ce) {
  typename E::state_type;
  { ce.serialize() } -> std::convertible_to<typename E::state_type>;
  e.deserialize(ce.serialize());
};
} // namespace detail

// Stream insertion/extraction for every family engine, satisfying the
// StreamSerializable clause of RandomNumberEngine. The state is written as
// space-separated decimal integers; format flags and fill are forced to a
// known state and restored so insertion does not perturb the stream
// (StreamPreservesFormatState).
template <class CharT, class Traits, class Engine>
  requires detail::XoshiroSerializable<Engine>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os, const Engine &engine) {
  using State = typename Engine::state_type;
  const State state = engine.serialize();
  const auto flags = os.flags();
  const auto fill = os.fill();
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(os.widen(' '));
  if constexpr (std::is_integral_v<State>) {
    os << state;
  } else {
    const CharT space = os.widen(' ');
    for (std::size_t i = 0; i < state.size(); ++i) {
      if (i != 0) {
        os << space;
      }
      os << state[i];
    }
  }
  os.flags(flags);
  os.fill(fill);
  return os;
}

template <class CharT, class Traits, class Engine>
  requires detail::XoshiroSerializable<Engine>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is, Engine &engine) {
  using State = typename Engine::state_type;
  State state = {};
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  if constexpr (std::is_integral_v<State>) {
    is >> state;
  } else {
    for (auto &element : state) {
      is >> element;
    }
  }
  is.flags(flags);
  if (is) {
    engine.deserialize(state);
  }
  return is;
}
} // namespace ni::random
