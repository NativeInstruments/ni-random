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

//! \file chacha_simd_equivalence_tests.cpp
//!
//! The core guarantee a golden value alone cannot give: on this machine, the
//! NATIVE SIMD block policy and the scalar reference policy produce
//! bit-identical ChaCha output streams. Instantiates the scalar-policy engine
//! and the native-SIMD-policy engine side by side in one translation unit
//! (impossible with the gist's whole-TU #ifdef selection — the reason the
//! import refactors the block into a template policy) and asserts they stay
//! equal across the states a single golden draw misses: many draws across block
//! boundaries, discard() landing mid-block and spanning blocks, a non-zero
//! stream, and seed-seq seeding.
//!
//! The checks are round-parameterized (TYPED_TEST over R = 8, 12, 20) so every
//! shipped ChaCha alias — ChaCha8, ChaCha12, ChaCha20 — is proven SIMD ↔ scalar
//! bit-identical, not just the 20-round default.
//!
//! Guarded by NI_RANDOM_CHACHA_HAS_SIMD (from detail/ChaChaBlock.hpp): on a
//! SIMD-less platform there is no native path to compare against, so every test
//! GTEST_SKIP()s with the platform named — a visible skip, never a silent pass.
//! Composed transitively with the forced-scalar anchors (scalar ↔ golden on
//! every platform), this validates all three paths: NEON ↔ scalar on ARM, SSE
//! ↔ scalar on x86 CI, with no machine ever compiling a foreign ISA.

#include <cstddef>
#include <cstdint>
#include <random>
#include <sstream>
#include <type_traits>

#include <gtest/gtest.h>

#include "ni/random/ChaCha.hpp"
#include "ni/random/detail/ChaChaBlock.hpp"

namespace ni::random::test {

#if NI_RANDOM_CHACHA_HAS_SIMD

#if defined(__ARM_NEON) || defined(__aarch64__)
constexpr const char *kSimdName = "NEON";
#else
constexpr const char *kSimdName = "SSE2/SSSE3";
#endif

//! The scalar- and SIMD-policy engines are distinct types, so the hidden-friend
//! operator== (same-type only) cannot compare them directly. Their == compares
//! the logical state (keysetup + ctr), which is exactly what operator<<
//! serializes regardless of block policy — so equal serialized forms means
//! equal logical state. This is the cross-type "stay ==" check the plan asks
//! for.
template <class A, class B>
::testing::AssertionResult sameLogicalState(const A &a, const B &b) {
  std::ostringstream sa;
  std::ostringstream sb;
  sa << a;
  sb << b;
  if (sa.str() == sb.str()) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure() << "logical state differs: [" << sa.str()
                                       << "] vs [" << sb.str() << "]";
}

//! Fixture parameterized by round count: pairs the forced-scalar engine with
//! the native-SIMD-policy engine for the same R so the bodies are written once
//! and run for every shipped alias.
template <class RoundsT> class ChaChaSimdEquivalence : public ::testing::Test {
protected:
  static constexpr std::size_t kRounds = RoundsT::value;
  using Scalar = ni::random::detail::ChaChaImpl<kRounds, detail::ScalarBlock>;
  using Simd = ni::random::detail::ChaChaImpl<kRounds, detail::DefaultBlock>;
};

using RoundTypes = ::testing::Types<std::integral_constant<std::size_t, 8>,
                                    std::integral_constant<std::size_t, 12>,
                                    std::integral_constant<std::size_t, 20>>;
TYPED_TEST_SUITE(ChaChaSimdEquivalence, RoundTypes);

//! Many consecutive draws, well past several block refills (ChaCha refills
//! every 16 words), keep the two policies in lockstep and == throughout.
TYPED_TEST(ChaChaSimdEquivalence, OutputStreamBitIdenticalAcrossBlocks) {
  typename TestFixture::Scalar scalar;
  typename TestFixture::Simd simd;
  ASSERT_TRUE(sameLogicalState(scalar, simd))
      << kSimdName << " R=" << TestFixture::kRounds;
  for (int i = 0; i < 10000; ++i) {
    ASSERT_EQ(scalar(), simd())
        << kSimdName << " R=" << TestFixture::kRounds << " draw " << i;
  }
  EXPECT_TRUE(sameLogicalState(scalar, simd))
      << kSimdName << " R=" << TestFixture::kRounds;
}

//! discard(z) agrees for z landing mid-block (not a multiple of 16) and
//! spanning many blocks; the engines stay equal and produce identical output.
TYPED_TEST(ChaChaSimdEquivalence, DiscardAgreesMidBlockAndMultiBlock) {
  for (const unsigned long long z : {1ull, 5ull, 16ull, 17ull, 1000ull}) {
    typename TestFixture::Scalar scalar;
    typename TestFixture::Simd simd;
    scalar.discard(z);
    simd.discard(z);
    EXPECT_TRUE(sameLogicalState(scalar, simd))
        << kSimdName << " R=" << TestFixture::kRounds << " discard(" << z
        << ")";
    for (int i = 0; i < 100; ++i) {
      EXPECT_EQ(scalar(), simd()) << kSimdName << " R=" << TestFixture::kRounds
                                  << " discard(" << z << ") draw " << i;
    }
  }
}

//! A non-zero stream (the ChaCha substream selector) feeds different input
//! words to the block function; the two policies must still agree.
TYPED_TEST(ChaChaSimdEquivalence, NonZeroStreamBitIdentical) {
  constexpr std::uint64_t seed = 0xC0FFEEu;
  constexpr std::uint64_t stream = 0x9E3779B97F4A7C15ull;
  typename TestFixture::Scalar scalar(seed, stream);
  typename TestFixture::Simd simd(seed, stream);
  ASSERT_TRUE(sameLogicalState(scalar, simd))
      << kSimdName << " R=" << TestFixture::kRounds;
  for (int i = 0; i < 2000; ++i) {
    ASSERT_EQ(scalar(), simd())
        << kSimdName << " R=" << TestFixture::kRounds << " draw " << i;
  }
}

//! Seed-seq seeding feeds an arbitrary key into the block function; the two
//! policies must agree there as well.
TYPED_TEST(ChaChaSimdEquivalence, SeedSeqSeededBitIdentical) {
  std::seed_seq seq1{1u, 2u, 3u, 4u, 5u};
  std::seed_seq seq2{1u, 2u, 3u, 4u, 5u};
  typename TestFixture::Scalar scalar(seq1);
  typename TestFixture::Simd simd(seq2);
  ASSERT_TRUE(sameLogicalState(scalar, simd))
      << kSimdName << " R=" << TestFixture::kRounds;
  for (int i = 0; i < 2000; ++i) {
    ASSERT_EQ(scalar(), simd())
        << kSimdName << " R=" << TestFixture::kRounds << " draw " << i;
  }
}

#else // !NI_RANDOM_CHACHA_HAS_SIMD

TEST(ChaChaSimdEquivalence, SkippedNoSimdPath) {
  GTEST_SKIP() << "no native ChaCha SIMD block policy on this platform "
                  "(NI_RANDOM_CHACHA_HAS_SIMD == 0); the scalar path is "
                  "covered by chacha_scalar_tests.cpp";
}

#endif

} // namespace ni::random::test
