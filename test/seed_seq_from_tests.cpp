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

//! \file seed_seq_from_tests.cpp
//!
//! Runtime tests for ni::random::SeedSeqFrom — the RNG->seed-source adapter
//! imported from pcg_extras. SeedSeqFrom is a *producer*, NOT a SeedSequence
//! (it lacks param() and a public result_type), so it is tested as a producer
//! only: deterministic generate() fill, ability to seed an engine, and size().
//!
//! Golden vector is derived-then-pinned, as elsewhere in the suite.

#include <array>
#include <cstdint>
#include <random>

#include <gtest/gtest.h>

#include "ni/random/SeedSeqFrom.hpp"
#include "ni/random/SeedSequence.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

namespace ni::random::test {

//! (1) Compile-time documentation of the producer/adapter status: SeedSeqFrom
//! is intentionally NOT a SeedSequence. Restated here (also pinned header-side)
//! so the negative is locally visible in the runtime suite.
static_assert(!SeedSequence<SeedSeqFrom<std::mt19937>>);

//! (2) Producer behavior: a SeedSeqFrom<std::mt19937> built with a fixed seed
//! fills a buffer deterministically via generate(), matching the pinned golden
//! vector (8 words from seed 12345).
TEST(SeedSeqFrom, GenerateMatchesGolden) {
  static constexpr std::array<std::uint32_t, 8> kGolden = {
      0xedfb51e2u, 0xe3e12de5u, 0x50fdfd1du, 0x21760881u,
      0x2f154da4u, 0x0a2dada9u, 0x345e0ffeu, 0xd391517eu};

  SeedSeqFrom<std::mt19937> from(12345u);
  std::array<std::uint32_t, 8> out{};
  from.generate(out.begin(), out.end());

  EXPECT_EQ(out, kGolden);
}

//! (2b) It can actually seed engines, both standard and ni::random, and does so
//! deterministically: two adapters built with the same fixed seed drive two
//! engines to identical output.
TEST(SeedSeqFrom, SeedsEnginesDeterministically) {
  {
    SeedSeqFrom<std::mt19937> a(777u);
    SeedSeqFrom<std::mt19937> b(777u);
    std::mt19937 ea(a);
    std::mt19937 eb(b);
    EXPECT_EQ(ea(), eb());
    EXPECT_EQ(ea(), eb());
  }
  {
    SeedSeqFrom<std::mt19937> a(777u);
    SeedSeqFrom<std::mt19937> b(777u);
    Xoshiro256PlusPlus ea(a);
    Xoshiro256PlusPlus eb(b);
    EXPECT_EQ(ea(), eb());
    EXPECT_EQ(ea(), eb());
  }
  // Different seeds drive an engine to different state.
  SeedSeqFrom<std::mt19937> a(1u);
  SeedSeqFrom<std::mt19937> b(2u);
  std::mt19937 ea(a);
  std::mt19937 eb(b);
  EXPECT_NE(ea(), eb());
}

//! (3) size() reports the documented bound for the wrapped RNG. std::mt19937's
//! max() is 2^32 - 1, which fits std::uint_least32_t, so size() returns that.
TEST(SeedSeqFrom, SizeReportsWrappedRngBound) {
  SeedSeqFrom<std::mt19937> from(1u);
  EXPECT_EQ(from.size(), std::size_t(std::mt19937::max()));
  EXPECT_EQ(from.size(), 4294967295u);
}

} // namespace ni::random::test
