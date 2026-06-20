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

//! \file normal_distribution_tests.cpp
//!
//! Instantiates the reusable distribution conformance suite
//! (DistributionConformanceSuite.hpp) for
//! ni::random::NormalDistribution<double>, plus a hand-written test of its
//! STATEFUL contract that the generic suite cannot express: reset() must drop
//! the cached partner value of the Marsaglia polar pair.

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "ni/random/NormalDistribution.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

#include "DistributionConformanceSuite.hpp"
#include "DistributionConformanceTraits.hpp"

namespace ni::random::test {

template <>
struct DistributionConformanceTraits<ni::random::NormalDistribution<double>> {
  using distribution_type = ni::random::NormalDistribution<double>;
  using witness_engine = ni::random::Xoshiro256PlusPlus;
  static constexpr std::uint64_t engine_seed = 42u;
  static constexpr distribution_type::param_type sample_param{3.0, 2.0};
  static constexpr distribution_type::param_type other_param{0.0, 1.0};
  static constexpr std::size_t golden_n = 10000;
  //! Derived-then-pinned: the 10000th draw of a default NormalDistribution
  //! (N(0, 1)) driven by a default Xoshiro256PlusPlus. The draw is
  //! deterministic, so the suite compares it with exact EXPECT_EQ.
  static constexpr distribution_type::result_type golden_value =
      0.5278436470154787;
};

INSTANTIATE_TYPED_TEST_SUITE_P(Normal, DistributionConformance,
                               ni::random::NormalDistribution<double>);

//! Stateful contract: after one draw the polar partner is cached; reset() must
//! drop it so the distribution again behaves like a freshly-constructed one.
TEST(NormalDistributionState, ResetDropsCachedValue) {
  using D = ni::random::NormalDistribution<double>;
  using Engine = ni::random::Xoshiro256PlusPlus;

  D d;
  Engine warmup(7u);
  (void)d(warmup); // one draw leaves a live cached value
  d.reset();

  // A reset distribution must match a freshly-constructed one: identical
  // sequence under equally-seeded engines.
  D fresh;
  Engine ea(123u);
  Engine eb(123u);
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(d(ea), fresh(eb));
  }
}

} // namespace ni::random::test
