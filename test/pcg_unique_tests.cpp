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

//! \file pcg_unique_tests.cpp
//!
//! Minimal, dedicated tests for the two address-seeded PCG unique-stream
//! engines, ni::random::Pcg32Unique and ni::random::Pcg64Unique.
//!
//! These engines are NON-REPRODUCIBLE by design: unique_stream derives the LCG
//! increment from the engine object's own memory address on every step and
//! stores nothing (detail/PcgEngine.hpp `unique_stream`). They are therefore
//! NOT asserted against RandomNumberEngine and NOT run through the generic
//! EngineConformanceSuite — see the file-level notes in Pcg32Unique.hpp /
//! Pcg64Unique.hpp and the "reproducibility trap" section of
//! plans/import-pcg-family.md.
//!
//! We test only the two properties that genuinely hold for an address-seeded
//! stream. Golden values, copy-reproducibility, and stream round-trip are
//! DELIBERATELY OMITTED: a copy or a deserialized engine lands at a new address
//! with a new stream, so those behaviors are broken by design — do not "fix"
//! the missing coverage.

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/Pcg32Unique.hpp"
#include "ni/random/Pcg64Unique.hpp"

namespace ni::random::test {
namespace {

//! Defining property of a unique-stream engine: two distinct instances draw
//! from different streams (their addresses differ), so their output sequences
//! must diverge.
template <typename E> void expectTwoInstancesDiffer() {
  E a;
  E b;
  bool diverged = false;
  for (int i = 0; i < 1000; ++i) {
    if (a() != b()) {
      diverged = true;
      break;
    }
  }
  EXPECT_TRUE(diverged)
      << "two distinct unique-stream instances produced identical sequences";
}

//! In-place discard equivalence: on a SINGLE instance (fixed address, hence a
//! fixed stream), discard(z) advances the state exactly as z calls to
//! operator(). seed() resets to a deterministic start state; because the object
//! and its address are unchanged across both runs, the stream constant is the
//! same, so the two runs must agree. This is the one advancement guarantee that
//! survives address-seeding — copies cannot be used here.
template <typename E> void expectInPlaceDiscardEquivalence() {
  for (const unsigned long long z : {0ull, 1ull, 7ull, 100000ull}) {
    E e;

    e.seed();
    e.discard(z);
    std::vector<typename E::result_type> viaDiscard;
    for (int i = 0; i < 100; ++i) {
      viaDiscard.push_back(e());
    }

    e.seed(); // same object, same address => same stream constant
    for (unsigned long long i = 0; i < z; ++i) {
      (void)e();
    }
    std::vector<typename E::result_type> viaLoop;
    for (int i = 0; i < 100; ++i) {
      viaLoop.push_back(e());
    }

    EXPECT_EQ(viaDiscard, viaLoop) << "discard(" << z << ") diverged from loop";
  }
}

TEST(Pcg32Unique, TwoInstancesDiffer) {
  expectTwoInstancesDiffer<ni::random::Pcg32Unique>();
}

TEST(Pcg32Unique, InPlaceDiscardEquivalence) {
  expectInPlaceDiscardEquivalence<ni::random::Pcg32Unique>();
}

TEST(Pcg64Unique, TwoInstancesDiffer) {
  expectTwoInstancesDiffer<ni::random::Pcg64Unique>();
}

TEST(Pcg64Unique, InPlaceDiscardEquivalence) {
  expectInPlaceDiscardEquivalence<ni::random::Pcg64Unique>();
}

} // namespace
} // namespace ni::random::test
