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

//! \file DistributionConformanceSuite.hpp
//!
//! A reusable, type-parametrized gtest suite that verifies the *semantic*
//! [rand.req.dist] requirements a concept cannot check. Instantiate it for any
//! distribution with INSTANTIATE_TYPED_TEST_SUITE_P after supplying a
//! DistributionConformanceTraits specialization. It is the distribution
//! analogue of EngineConformanceSuite.hpp.
//!
//! Scope: interface conformance only. Like the engine suite, this verifies NO
//! statistical / quality properties (no mean/variance assertions, no
//! TestU01/PractRand). Whether a distribution is correctly shaped is the
//! algorithm author's claim; here we check the [rand.req.dist] surface and its
//! semantics (reproducibility, param round-trip, reset, stream round-trip,
//! min/max bounds, golden value).
//!
//! Witness: every test drives the distribution with Traits::witness_engine
//! (ni::random::Xoshiro256PlusPlus) — distinct from the std::minstd_rand
//! witness pinned inside the RandomNumberDistribution concept. The stream
//! dimension is probed with char and wchar_t.
//!
//! Determinism: the UTL distributions are platform-reproducible, so equal
//! distributions driven by equally-seeded engines produce bit-identical draws;
//! the suite compares draws (including floating-point) with EXPECT_EQ.

#include <cstddef>
#include <cstdint>
#include <sstream>

#include <gtest/gtest.h>

#include "ni/random/RandomNumberDistribution.hpp"

#include "DistributionConformanceTraits.hpp"

namespace ni::random::test {

//! Compile-time fold over a fixed list of witness types: invokes a generic
//! lambda once per type W. The callable must be a templated operator() (use
//! [&]<typename W>() { ... }).
template <typename... Ws, typename F> void forEachType(F &&f) {
  (f.template operator()<Ws>(), ...);
}

//! Fixture template. Pulls all distribution-specific config from the traits.
template <typename D> class DistributionConformance : public ::testing::Test {
protected:
  using Traits = DistributionConformanceTraits<D>;
};

TYPED_TEST_SUITE_P(DistributionConformance);

//! [rand.req.dist]: the type models the distribution concept. Compile-time, but
//! kept in the suite so it travels with every instantiation.
TYPED_TEST_P(DistributionConformance, ConceptSatisfied) {
  static_assert(RandomNumberDistribution<TypeParam>);
  SUCCEED();
}

//! Reproducibility: two equal distributions driven by two equally-seeded
//! engines produce identical next-N draws.
TYPED_TEST_P(DistributionConformance, Reproducible) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  using Engine = typename Traits::witness_engine;
  D a(Traits::sample_param);
  D b(Traits::sample_param);
  Engine ea(Traits::engine_seed);
  Engine eb(Traits::engine_seed);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a(ea), b(eb));
  }
}

//! Param round-trip: param(p) then param() returns p, and a param-constructed
//! distribution reports the same param.
TYPED_TEST_P(DistributionConformance, ParamRoundTrip) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  D d;
  d.param(Traits::sample_param);
  EXPECT_EQ(d.param(), Traits::sample_param);
  EXPECT_EQ(D(Traits::sample_param).param(), Traits::sample_param);
}

//! A distribution built from a param_type equals one default-constructed then
//! param()'d, and both yield the same sequence under equally-seeded engines.
TYPED_TEST_P(DistributionConformance, ParamConstructionEquivalence) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  using Engine = typename Traits::witness_engine;
  D a(Traits::sample_param);
  D b;
  b.param(Traits::sample_param);
  EXPECT_EQ(a, b);
  Engine ea(Traits::engine_seed);
  Engine eb(Traits::engine_seed);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a(ea), b(eb));
  }
}

//! min()/max(): every draw (with sample_param) lies in [min, max], and
//! min() <= max().
TYPED_TEST_P(DistributionConformance, MinMaxBounds) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  using Engine = typename Traits::witness_engine;
  D d(Traits::sample_param);
  EXPECT_LE(d.min(), d.max());
  Engine e(Traits::engine_seed);
  for (int i = 0; i < 10000; ++i) {
    const auto v = d(e);
    EXPECT_GE(v, d.min());
    EXPECT_LE(v, d.max());
  }
}

//! Equality/inequality of distributions and of their param_types.
TYPED_TEST_P(DistributionConformance, EqualityInequality) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  EXPECT_EQ(D(Traits::sample_param), D(Traits::sample_param));
  EXPECT_NE(D(Traits::sample_param), D(Traits::other_param));
  EXPECT_EQ(Traits::sample_param, Traits::sample_param);
  EXPECT_NE(Traits::sample_param, Traits::other_param);
}

//! reset() leaves param() unchanged and is harmless to call. (Deeper
//! stateful-reset behavior for NormalDistribution is covered in its own TU.)
TYPED_TEST_P(DistributionConformance, ResetKeepsParam) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  D d(Traits::sample_param);
  d.reset();
  EXPECT_EQ(d.param(), Traits::sample_param);
}

//! Round-trip postcondition, generalized over char widths. Advance the
//! distribution by an ODD number of draws (so a stateful NormalDistribution has
//! a live cached value), stream out, read into a fresh distribution; the two
//! must be equal and produce identical next-N draws under equally-seeded
//! engines.
TYPED_TEST_P(DistributionConformance, StreamRoundTrip) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  using Engine = typename Traits::witness_engine;
  forEachType<char, wchar_t>([&]<typename C>() {
    D x(Traits::sample_param);
    Engine ex(Traits::engine_seed);
    for (int i = 0; i < 7;
         ++i) { // odd: leaves a live cache for NormalDistribution
      (void)x(ex);
    }
    std::basic_stringstream<C> ss;
    ss << x;
    D y;
    ss >> y;
    EXPECT_EQ(x, y);
    Engine e1(Traits::engine_seed);
    Engine e2(Traits::engine_seed);
    for (int i = 0; i < 1000; ++i) {
      EXPECT_EQ(x(e1), y(e2));
    }
  });
}

//! fmtflags/fill postcondition: operator<< must not alter the stream's format
//! state. Generalized over char widths.
TYPED_TEST_P(DistributionConformance, StreamPreservesFormatState) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  forEachType<char, wchar_t>([&]<typename C>() {
    D x(Traits::sample_param);
    std::basic_stringstream<C> ss;
    const auto flags = ss.flags();
    const auto fill = ss.fill();
    ss << x;
    EXPECT_EQ(ss.flags(), flags);
    EXPECT_EQ(ss.fill(), fill);
  });
}

//! Regression anchor: the golden_n-th draw of a default distribution driven by
//! a default-constructed witness engine must equal the pinned golden_value.
TYPED_TEST_P(DistributionConformance, GoldenValue) {
  using D = TypeParam;
  using Traits = typename TestFixture::Traits;
  using Engine = typename Traits::witness_engine;
  D d;
  Engine e;
  typename D::result_type last{};
  for (std::size_t i = 0; i < Traits::golden_n; ++i) {
    last = d(e);
  }
  EXPECT_EQ(last, Traits::golden_value);
}

REGISTER_TYPED_TEST_SUITE_P(DistributionConformance, ConceptSatisfied,
                            Reproducible, ParamRoundTrip,
                            ParamConstructionEquivalence, MinMaxBounds,
                            EqualityInequality, ResetKeepsParam,
                            StreamRoundTrip, StreamPreservesFormatState,
                            GoldenValue);

} // namespace ni::random::test
