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

//! \file EngineConformanceSuite.hpp
//!
//! A reusable, type-parametrized gtest suite that verifies the *semantic*
//! [rand.req.eng] requirements a concept cannot check. Instantiate it for any
//! engine with INSTANTIATE_TYPED_TEST_SUITE_P after supplying an
//! EngineConformanceTraits specialization.
//!
//! Oracle assumption: std::minstd_rand is treated as a known-conforming engine.
//! Every test here MUST pass against it. If a test fails on std::minstd_rand,
//! the test is wrong, not the engine.
//!
//! Genericity is verified by *canonical + adversarially-minimal* witnesses
//! only. Proving an engine "works for all SeedSequence / charT types" is
//! impossible at runtime, the same honest-limitation class as "statistical
//! quality is unverifiable" (which this suite also does not test — interface
//! conformance only, no TestU01/PractRand). The seed-sequence dimension is
//! probed with std::seed_seq (canonical) and MinimalSeedSeq (exactly the
//! required interface, nothing more); the stream dimension with char and
//! wchar_t.
//!
//! NOTE on static_assert(SeedableBy<...>): a requires-check of E(q)/seed(q)
//! validates only the signature/constraint — it does NOT instantiate the
//! constructor/seed body, so a latent hard error there would pass the concept
//! and only blow up when called. Each generalized test therefore *also*
//! actually constructs/seeds/streams in real code to force body instantiation.

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <random>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/RandomNumberEngine.hpp"

#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

//! Compile-time fold over a fixed list of witness types: invokes a generic
//! lambda once per type W. The callable must be a templated operator() (use
//! [&]<typename W>() { ... }).
template <typename... Ws, typename F> void forEachType(F &&f) {
  (f.template operator()<Ws>(), ...);
}

//! A seed sequence with EXACTLY the [rand.req.seedseq] interface and nothing
//! more — no std::seed_seq-specific extras. Catches an engine that relies on
//! seed_seq internals. generate() produces a distinctive, content-dependent
//! fill so a broken engine cannot pass vacuously on a constant/empty fill.
struct MinimalSeedSeq {
  using result_type = std::uint_least32_t;

  MinimalSeedSeq() = default;

  template <typename It> MinimalSeedSeq(It b, It e) : m_data(b, e) {}

  MinimalSeedSeq(std::initializer_list<result_type> il) : m_data(il) {}

  template <typename RandomIt> void generate(RandomIt first, RandomIt last) {
    result_type acc = 0x9E3779B9u ^ static_cast<result_type>(m_data.size());
    for (auto it = first; it != last; ++it) {
      for (auto s : m_data) {
        acc = acc * 1664525u + s + 1013904223u;
      }
      *it = (acc ^= acc >> 15);
    }
  }

  std::size_t size() const { return m_data.size(); }

  template <typename OutIt> void param(OutIt d) const {
    std::copy(m_data.begin(), m_data.end(), d);
  }

private:
  std::vector<result_type> m_data;
};

static_assert(SeedSequence<MinimalSeedSeq>,
              "MinimalSeedSeq must itself model the SeedSequence concept");

//! Fixture template. Pulls all engine-specific config from the traits.
template <typename E> class EngineConformance : public ::testing::Test {
protected:
  using Traits = EngineConformanceTraits<E>;
};

TYPED_TEST_SUITE_P(EngineConformance);

//! [rand.req.eng]: the type models the engine concept. Compile-time, but kept
//! in the suite so it travels with every instantiation.
TYPED_TEST_P(EngineConformance, ConceptSatisfied) {
  static_assert(RandomNumberEngine<TypeParam>);
  SUCCEED();
}

//! Row E(): two default-constructed engines compare equal and produce
//! identical next-N outputs.
TYPED_TEST_P(EngineConformance, DefaultConstructionEqual) {
  using E = TypeParam;
  E a;
  E b;
  EXPECT_EQ(a, b);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a(), b());
  }
}

//! Rows E(x), ==, !=: a copy compares equal, then diverges once the original
//! advances.
TYPED_TEST_P(EngineConformance, CopyEqualThenDiverge) {
  using E = TypeParam;
  E x;
  E y(x);
  EXPECT_EQ(x, y);
  (void)x();
  EXPECT_NE(x, y);
}

//! Operationalizes == semantics (Sx == Sy): equal engines yield elementwise
//! equal sequences.
TYPED_TEST_P(EngineConformance, EqualImpliesSameSequence) {
  using E = TypeParam;
  E x;
  (void)x(); // move off the default state
  E y(x);
  ASSERT_EQ(x, y);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(x(), y());
  }
}

//! Rows E(s), seed(s): the postcondition is e == E(s). A value-seeded engine
//! equals one default-constructed then seed(s)'d, and two E(s) agree.
TYPED_TEST_P(EngineConformance, SeedValueReproducible) {
  using E = TypeParam;
  const auto seed = TestFixture::Traits::sample_seed;
  // Two value-seeded engines (one via ctor, one via seed(s)) are equal and
  // stay in lockstep. Compare a third fresh E(seed) before any draws, since
  // the lockstep loop below advances both a and b.
  E a(seed);
  E b;
  b.seed(seed);
  EXPECT_EQ(a, b);
  E c(seed);
  EXPECT_EQ(a, c);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a(), b());
  }
}

//! Row seed(): postcondition is *this == E(). Advance, then reset.
TYPED_TEST_P(EngineConformance, SeedDefaultResets) {
  using E = TypeParam;
  E a;
  for (int i = 0; i < 100; ++i) {
    (void)a();
  }
  a.seed();
  EXPECT_EQ(a, E());
}

//! Rows E(q), seed(q), generalized over seed-sequence witnesses (Step 3a).
//! For each witness S: equal-content sequences produce equal engines and
//! identical output. Forces ctor and seed() bodies, not just the constraint.
TYPED_TEST_P(EngineConformance, SeedSeqReproducible) {
  using E = TypeParam;
  forEachType<std::seed_seq, MinimalSeedSeq>([&]<typename S>() {
    static_assert(SeedableBy<E, S>); // constraint admits it...
    const std::initializer_list<std::uint_least32_t> data{1u, 2u, 3u, 4u};
    S q1(data);
    S q2(data);
    E a(q1); // ...forces the ctor body
    E b;
    b.seed(q2); // ...forces the seed() body
    EXPECT_EQ(a, b);
    for (int i = 0; i < 1000; ++i) {
      EXPECT_EQ(a(), b());
    }
    // The seed did something: a seed-seq-seeded engine differs from
    // default. (Holds for any conforming engine; no gold value needed.)
    E def;
    EXPECT_NE(a, def);
  });
}

//! Rows discard(z): discard(z) must equal z invocations of operator(). This
//! is the test that catches a broken fast-skip. Checks both resulting state
//! and subsequent output for every z in the traits' discard_counts.
TYPED_TEST_P(EngineConformance, DiscardEquivalentToLoop) {
  using E = TypeParam;
  for (const auto z : TestFixture::Traits::discard_counts) {
    E skipped;
    E looped;
    skipped.discard(z);
    for (unsigned long long i = 0; i < z; ++i) {
      (void)looped();
    }
    EXPECT_EQ(skipped, looped) << "discard(" << z << ") state mismatch";
    for (int i = 0; i < 100; ++i) {
      EXPECT_EQ(skipped(), looped()) << "discard(" << z << ") output mismatch";
    }
  }
}

//! discard(0) is a no-op.
TYPED_TEST_P(EngineConformance, DiscardZeroIsNoop) {
  using E = TypeParam;
  E a;
  (void)a(); // arbitrary non-default state
  E before(a);
  a.discard(0);
  EXPECT_EQ(a, before);
}

//! [rand.req.eng] min()/max(): every draw lies in [min, max] and min < max.
TYPED_TEST_P(EngineConformance, RangeWithinBounds) {
  using E = TypeParam;
  EXPECT_LT(E::min(), E::max());
  E a;
  for (int i = 0; i < 10000; ++i) {
    const auto v = a();
    EXPECT_GE(v, E::min());
    EXPECT_LE(v, E::max());
  }
}

//! Round-trip postcondition x == v, generalized over char widths (Step 3a).
//! Write an advanced engine via operator<<, read into a fresh engine via
//! operator>>; the two must be equal and produce identical next-N outputs.
TYPED_TEST_P(EngineConformance, StreamRoundTrip) {
  using E = TypeParam;
  forEachType<char, wchar_t>([&]<typename C>() {
    E x;
    for (int i = 0; i < 50; ++i) {
      (void)x();
    }
    std::basic_stringstream<C> ss;
    ss << x;
    E y;
    ss >> y;
    EXPECT_EQ(x, y);
    for (int i = 0; i < 1000; ++i) {
      EXPECT_EQ(x(), y());
    }
  });
}

//! fmtflags/fill postcondition: operator<< must not alter the stream's
//! format state. Generalized over char widths.
TYPED_TEST_P(EngineConformance, StreamPreservesFormatState) {
  using E = TypeParam;
  forEachType<char, wchar_t>([&]<typename C>() {
    E x;
    for (int i = 0; i < 50; ++i) {
      (void)x();
    }
    std::basic_stringstream<C> ss;
    const auto flags = ss.flags();
    const auto fill = ss.fill();
    ss << x;
    EXPECT_EQ(ss.flags(), flags);
    EXPECT_EQ(ss.fill(), fill);
  });
}

//! Regression anchor: the golden_n-th output of a default-constructed engine
//! must equal the pinned golden_value. Guards against accidental changes to
//! the engine or its stream/serialization behavior.
TYPED_TEST_P(EngineConformance, GoldenValue) {
  using E = TypeParam;
  E a;
  typename E::result_type last{};
  for (std::size_t i = 0; i < TestFixture::Traits::golden_n; ++i) {
    last = a();
  }
  EXPECT_EQ(last, TestFixture::Traits::golden_value);
}

REGISTER_TYPED_TEST_SUITE_P(EngineConformance, ConceptSatisfied,
                            DefaultConstructionEqual, CopyEqualThenDiverge,
                            EqualImpliesSameSequence, SeedValueReproducible,
                            SeedDefaultResets, SeedSeqReproducible,
                            DiscardEquivalentToLoop, DiscardZeroIsNoop,
                            RangeWithinBounds, StreamRoundTrip,
                            StreamPreservesFormatState, GoldenValue);

} // namespace ni::random::test
