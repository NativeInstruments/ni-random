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

//! \file minstd_rand_tests.cpp
//!
//! Instantiates the reusable conformance suite for the reference engine,
//! std::minstd_rand. This file is the template downstream engines copy: define
//! the engine's EngineConformanceTraits specialization (here, beside the
//! instantiation) plus one INSTANTIATE_TYPED_TEST_SUITE_P line, and nothing
//! else.
//!
//! It also holds the NEGATIVE tests that prove the concept's gate actually
//! bites, and a standard-consumer integration check.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/RandomNumberEngine.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

//! std::minstd_rand — the reference engine. Its 10000th output from a
//! default-constructed instance is pinned by the standard at 399268537. We
//! treat this as *derived, then pinned*: the GoldenValue test regenerates it
//! and fails loudly on disagreement rather than trusting the constant blindly.
//! Hand-written rather than via NI_RANDOM_XOSHIRO_TRAITS (that macro hardcodes
//! the ni::random:: namespace and cannot express std::minstd_rand).
template <> struct EngineConformanceTraits<std::minstd_rand> {
  using engine_type = std::minstd_rand;

  //! z values for discard() tests: zero, small, and a large value to
  //! exercise any fast jump-ahead path.
  static constexpr unsigned long long discard_counts[] = {0ull, 1ull, 7ull,
                                                          100000ull};

  static constexpr std::size_t golden_n = 10000;
  static constexpr std::minstd_rand::result_type golden_value = 399268537u;

  //! A non-default seed for seed-from-value tests.
  static constexpr std::minstd_rand::result_type sample_seed = 42u;

  //! Optional extension point: additional engine-specific SeedSequence
  //! witnesses the suite should probe, beyond the shared ones. Empty here.
  using extra_seed_seqs = std::tuple<>;
};

INSTANTIATE_TYPED_TEST_SUITE_P(MinstdRand, EngineConformance, std::minstd_rand);

} // namespace ni::random::test

namespace {

using ni::random::RandomNumberEngine;
using ni::random::SeedableBy;

//! Negative test #1: a type missing discard() must be rejected. It otherwise
//! mimics an engine's surface so we know discard specifically is what fails.
struct NoDiscard {
  using result_type = std::uint32_t;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0xFFFFFFFFu; }
  result_type operator()() { return 0; }
  void seed() {}
  void seed(result_type) {}
  void seed(std::seed_seq &) {}
  NoDiscard() = default;
  NoDiscard(result_type) {}
  NoDiscard(std::seed_seq &) {}
  bool operator==(const NoDiscard &) const = default;
  // no discard()
};

//! Negative test #2: a type with everything EXCEPT the seed-sequence overload.
//! This proves the SeedableBy clause genuinely contributes to the gate rather
//! than riding along — it must be rejected even though discard() is present.
struct NoSeedSeq {
  using result_type = std::uint32_t;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0xFFFFFFFFu; }
  result_type operator()() { return 0; }
  void seed() {}
  void seed(result_type) {}
  void discard(unsigned long long) {}
  NoSeedSeq() = default;
  NoSeedSeq(result_type) {}
  bool operator==(const NoSeedSeq &) const = default;
  // no seed_seq ctor / seed(seed_seq&) overload
};

static_assert(RandomNumberEngine<std::minstd_rand>,
              "reference engine must satisfy the concept");
static_assert(!RandomNumberEngine<NoDiscard>,
              "a type without discard() must be rejected");
static_assert(!RandomNumberEngine<NoSeedSeq>,
              "a type without the seed-sequence overload must be rejected");
static_assert(!SeedableBy<NoSeedSeq, std::seed_seq>,
              "SeedableBy must reject a type lacking the seed-seq overload");

TEST(RandomNumberEngineConcept, AcceptsReferenceRejectsBrokenStubs) {
  EXPECT_TRUE(RandomNumberEngine<std::minstd_rand>);
  EXPECT_FALSE(RandomNumberEngine<NoDiscard>);
  EXPECT_FALSE(RandomNumberEngine<NoSeedSeq>);
}

//! Independent cross-check: feed the engine to standard consumers. The pass
//! condition is that this compiles and runs without UB.
TEST(MinstdRandStandardConsumers, AdaptersAndDistributions) {
  std::independent_bits_engine<std::minstd_rand, 32, std::uint32_t> ibe;
  (void)ibe();

  std::discard_block_engine<std::minstd_rand, 16, 8> dbe;
  (void)dbe();

  std::minstd_rand engine;
  std::uniform_int_distribution<int> dist(0, 99);
  for (int i = 0; i < 100; ++i) {
    const int v = dist(engine);
    EXPECT_GE(v, 0);
    EXPECT_LE(v, 99);
  }

  std::vector<int> data(50);
  std::iota(data.begin(), data.end(), 0);
  std::shuffle(data.begin(), data.end(), engine);
  std::vector<int> sorted(data);
  std::sort(sorted.begin(), sorted.end());
  std::vector<int> expected(50);
  std::iota(expected.begin(), expected.end(), 0);
  EXPECT_EQ(sorted, expected); // shuffle is a permutation
}
} // namespace
