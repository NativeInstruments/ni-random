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

//! \file philox_set_counter_tests.cpp
//!
//! Covers the C++26 [rand.eng.philox] set_counter API, which the generic
//! EngineConformanceSuite does not exercise (set_counter is Philox-specific,
//! the counter-based analogue of the xoshiro family's jump()/longJump()).
//! Tested generically over both predefined engines via a typed fixture.
//!
//! set_counter maps X_j = c_{n-1-j} mod 2^w with i = n - 1, so the next n
//! outputs are the Philox(K, X) batch for the counter just set. Because the
//! default engine starts at counter 0 and advances by one batch every n
//! outputs, positioning to counter Z via set_counter({0, 0, 0, Z}) makes the
//! next n outputs equal those of an engine that arrived at counter Z by drawing
//! Z * n values -- the property these tests pin.

#include <array>
#include <cstddef>

#include <gtest/gtest.h>

#include "ni/random/Philox4x32.hpp"
#include "ni/random/Philox4x64.hpp"

namespace ni::random::test {

template <typename E> class PhiloxSetCounter : public ::testing::Test {
protected:
  using result_type = typename E::result_type;
  static constexpr std::size_t n = 4;

  //! A little-endian counter value Z (Z < 2^w) as the set_counter argument:
  //! set_counter assigns X_j = c_{n-1-j}, so c = {0, 0, 0, Z} yields the
  //! counter X = {Z, 0, 0, 0}.
  static std::array<result_type, n> counterArg(result_type z) {
    return {0, 0, 0, z};
  }
};

using PhiloxEngines =
    ::testing::Types<ni::random::Philox4x32, ni::random::Philox4x64>;
TYPED_TEST_SUITE(PhiloxSetCounter, PhiloxEngines);

//! Positioning to counter Z via set_counter reproduces the batch an engine
//! reaches by drawing Z * n values forward -- counter positioning is
//! reproducible and independent of how the engine arrived there.
TYPED_TEST(PhiloxSetCounter, PositionsToBatchReachedByDrawing) {
  using E = TypeParam;
  constexpr typename TestFixture::result_type kBatch = 7;

  E positioned;
  positioned.set_counter(TestFixture::counterArg(kBatch));

  E advanced;
  advanced.discard(kBatch * TestFixture::n); // draw the first kBatch batches

  for (std::size_t k = 0; k < TestFixture::n; ++k) {
    EXPECT_EQ(positioned(), advanced()) << "batch element " << k;
  }
}

//! Two engines with the same key positioned to DIFFERENT counters produce
//! different output -- the parallel-substream property set_counter exists for.
TYPED_TEST(PhiloxSetCounter, DistinctCountersDiffer) {
  using E = TypeParam;
  E a;
  E b;
  a.set_counter(TestFixture::counterArg(5));
  b.set_counter(TestFixture::counterArg(9));

  bool anyDifferent = false;
  for (std::size_t k = 0; k < TestFixture::n; ++k) {
    if (a() != b()) {
      anyDifferent = true;
    }
  }
  EXPECT_TRUE(anyDifferent);
}

//! Belt-and-suspenders against the multiword increment: set_counter(c) then
//! advancing k batches equals set_counter(c + k).
TYPED_TEST(PhiloxSetCounter, AdvanceEqualsHigherCounter) {
  using E = TypeParam;
  E advanced;
  advanced.set_counter(TestFixture::counterArg(2));
  advanced.discard(3 * TestFixture::n); // advance three batches: 2 -> 5

  E direct;
  direct.set_counter(TestFixture::counterArg(5));

  for (std::size_t k = 0; k < TestFixture::n; ++k) {
    EXPECT_EQ(advanced(), direct()) << "batch element " << k;
  }
}

//! set_counter leaves i = n - 1, so a fresh batch begins on the next call: an
//! engine reset to counter 0 via set_counter matches a freshly default-seeded
//! engine (which also starts at counter 0).
TYPED_TEST(PhiloxSetCounter, CounterZeroMatchesFreshEngine) {
  using E = TypeParam;
  E fresh;
  E reset;
  reset.set_counter(TestFixture::counterArg(0));
  for (int k = 0; k < 1000; ++k) {
    EXPECT_EQ(fresh(), reset());
  }
}

} // namespace ni::random::test
