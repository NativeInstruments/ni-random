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
/*
 * Random-Number Utilities (randutil)
 *     Addresses common issues with C++11 random number generation.
 *     Makes good seeding easier, and makes using RNGs easy while retaining
 *     all the power.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2022 Melissa E. O'Neill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
// clang-format on

//! \file SeedSeqFE.hpp
//!
//! Verbatim import of randutils::seed_seq_fe (the "Fixed Entropy" seed
//! sequence) from randutils.hpp, by Melissa E. O'Neill. The algorithm body is
//! unmodified; only the enclosing namespace (randutils -> ni::random), the
//! C++20-unconditional constexpr (the upstream RANDUTILS_GENERALIZED_CONSTEXPR
//! macro is always-on here), and the file organization differ from upstream.
//! Only seed_seq_fe and its 128-/256-bit aliases are imported; auto_seeded and
//! random_generator are intentionally left behind (seed_seq_fe is
//! self-contained). See plans/import-improved-seed-sequence.md.
//!
//! seed_seq_fe is a drop-in replacement for std::seed_seq that avoids
//! std::seed_seq's bias / non-injectivity, performs better in empirical
//! statistical tests, and is faster at normal sizes. In normal use it is
//! reached through the SeedSeqFE128 / SeedSeqFE256 aliases.
//!
//! Design rationale and the bias it fixes:
//!   https://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
//!   https://www.pcg-random.org/posts/cpp-seeding-surprises.html

#pragma once
#include "ni/random/SeedSequence.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace ni::random {

//! seed_seq_fe implements a fixed-entropy seed sequence; it conforms to all the
//! requirements of the [rand.req.seedseq] Seed Sequence concept.
//!
//! seed_seq_fe<N> seeds based on a store of N * 32 bits of entropy; typically
//! it is initialized with N or more integers. With M seed integers as input it
//! is bias-free when M >= N (the entropy store is a bijection of the inputs)
//! and exhibits full avalanche (a single-bit input change flips every output
//! bit with 50% probability). Because the entropy store is fixed-size it never
//! allocates and cannot throw. See the design rationale linked in the file doc.
template <std::size_t count = 4, typename IntRep = std::uint32_t,
          std::size_t mix_rounds = 1 + (count <= 2)>
struct seed_seq_fe {
public:
  // types
  typedef IntRep result_type;

private:
  static constexpr std::uint32_t INIT_A = 0x43b0d7e5;
  static constexpr std::uint32_t MULT_A = 0x931e8875;

  static constexpr std::uint32_t INIT_B = 0x8b51f9dd;
  static constexpr std::uint32_t MULT_B = 0x58f38ded;

  static constexpr std::uint32_t MIX_MULT_L = 0xca01f9dd;
  static constexpr std::uint32_t MIX_MULT_R = 0x4973f715;
  static constexpr std::uint32_t XSHIFT = sizeof(IntRep) * 8 / 2;

  static constexpr IntRep fast_exp(IntRep x, IntRep power) {
    IntRep result = IntRep(1);
    IntRep multiplier = x;
    while (power != IntRep(0)) {
      IntRep thismult = power & IntRep(1) ? multiplier : IntRep(1);
      result *= thismult;
      power >>= 1;
      multiplier *= multiplier;
    }
    return result;
  }

  std::array<IntRep, count> mixer_;

  template <typename InputIter>
  void mix_entropy(InputIter begin, InputIter end);

public:
  seed_seq_fe(const seed_seq_fe &) = delete;
  void operator=(const seed_seq_fe &) = delete;

  //! ni-random conformance addition (NOT in upstream randutils): upstream
  //! seed_seq_fe declares only the initializer_list / iterator-pair
  //! constructors, so it has no default constructor and would fail the
  //! [rand.req.seedseq] / ni::random::SeedSequence default-construction
  //! requirement. Default construction seeds from an empty entropy range,
  //! yielding the fixed initial state shared by all default-constructed
  //! instances that the standard mandates. The algorithm bodies are unchanged.
  seed_seq_fe() {
    IntRep *const none = nullptr;
    seed(none, none);
  }

  template <typename T> seed_seq_fe(std::initializer_list<T> init) {
    seed(init.begin(), init.end());
  }

  template <typename InputIter> seed_seq_fe(InputIter begin, InputIter end) {
    seed(begin, end);
  }

  // generating functions
  template <typename RandomAccessIterator>
  void generate(RandomAccessIterator first, RandomAccessIterator last) const;

  static constexpr std::size_t size() { return count; }

  template <typename OutputIterator> void param(OutputIterator dest) const;

  template <typename InputIter> void seed(InputIter begin, InputIter end) {
    mix_entropy(begin, end);
    // For very small sizes, we do some additional mixing.  For normal
    // sizes, this loop never performs any iterations.
    for (std::size_t i = 1; i < mix_rounds; ++i)
      stir();
  }

  seed_seq_fe &stir() {
    mix_entropy(mixer_.begin(), mixer_.end());
    return *this;
  }
};

template <std::size_t count, typename IntRep, std::size_t r>
template <typename InputIter>
void seed_seq_fe<count, IntRep, r>::mix_entropy(InputIter begin,
                                                InputIter end) {
  auto hash_const = INIT_A;
  auto hash = [&](IntRep value) {
    value ^= hash_const;
    hash_const *= MULT_A;
    value *= hash_const;
    value ^= value >> XSHIFT;
    return value;
  };
  auto mix = [](IntRep x, IntRep y) {
    IntRep result = MIX_MULT_L * x - MIX_MULT_R * y;
    result ^= result >> XSHIFT;
    return result;
  };

  InputIter current = begin;
  for (auto &elem : mixer_) {
    if (current != end)
      elem = hash(*current++);
    else
      elem = hash(0U);
  }
  for (auto &src : mixer_)
    for (auto &dest : mixer_)
      if (&src != &dest)
        dest = mix(dest, hash(src));
  for (; current != end; ++current)
    for (auto &dest : mixer_)
      dest = mix(dest, hash(*current));
}

template <std::size_t count, typename IntRep, std::size_t mix_rounds>
template <typename OutputIterator>
void seed_seq_fe<count, IntRep, mix_rounds>::param(OutputIterator dest) const {
  const IntRep INV_A = fast_exp(MULT_A, IntRep(-1));
  const IntRep MIX_INV_L = fast_exp(MIX_MULT_L, IntRep(-1));

  auto mixer_copy = mixer_;
  for (std::size_t round = 0; round < mix_rounds; ++round) {
    // Advance to the final value.  We'll backtrack from that.
    auto hash_const = INIT_A * fast_exp(MULT_A, IntRep(count * count));

    for (auto src = mixer_copy.rbegin(); src != mixer_copy.rend(); ++src)
      for (auto dest = mixer_copy.rbegin(); dest != mixer_copy.rend(); ++dest)
        if (src != dest) {
          IntRep revhashed = *src;
          auto mult_const = hash_const;
          hash_const *= INV_A;
          revhashed ^= hash_const;
          revhashed *= mult_const;
          revhashed ^= revhashed >> XSHIFT;
          IntRep unmixed = *dest;
          unmixed ^= unmixed >> XSHIFT;
          unmixed += MIX_MULT_R * revhashed;
          unmixed *= MIX_INV_L;
          *dest = unmixed;
        }
    for (auto i = mixer_copy.rbegin(); i != mixer_copy.rend(); ++i) {
      IntRep unhashed = *i;
      unhashed ^= unhashed >> XSHIFT;
      unhashed *= fast_exp(hash_const, IntRep(-1));
      hash_const *= INV_A;
      unhashed ^= hash_const;
      *i = unhashed;
    }
  }
  std::copy(mixer_copy.begin(), mixer_copy.end(), dest);
}

template <std::size_t count, typename IntRep, std::size_t mix_rounds>
template <typename RandomAccessIterator>
void seed_seq_fe<count, IntRep, mix_rounds>::generate(
    RandomAccessIterator dest_begin, RandomAccessIterator dest_end) const {
  auto src_begin = mixer_.begin();
  auto src_end = mixer_.end();
  auto src = src_begin;
  auto hash_const = INIT_B;
  for (auto dest = dest_begin; dest != dest_end; ++dest) {
    auto dataval = *src;
    if (++src == src_end)
      src = src_begin;
    dataval ^= hash_const;
    hash_const *= MULT_B;
    dataval *= hash_const;
    dataval ^= dataval >> XSHIFT;
    *dest = dataval;
  }
}

//! Convenience aliases for 128- and 256-bit entropy stores respectively. These
//! are the names client code should use; both are better-mixing drop-in
//! replacements for std::seed_seq.
using SeedSeqFE128 = seed_seq_fe<4, std::uint32_t>;
using SeedSeqFE256 = seed_seq_fe<8, std::uint32_t>;

//! seed_seq_fe is a genuine [rand.req.seedseq] seed sequence: it has an
//! unsigned-integral result_type >= 32 bits, size(), generate(first, last), and
//! a const param(out). Gate both aliases against the contract.
static_assert(SeedSequence<SeedSeqFE128>);
static_assert(SeedSequence<SeedSeqFE256>);

} // namespace ni::random
