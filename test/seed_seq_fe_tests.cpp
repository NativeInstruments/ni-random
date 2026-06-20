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

//! \file seed_seq_fe_tests.cpp
//!
//! Runtime conformance tests for the improved fixed-entropy seed sequence
//! ni::random::seed_seq_fe (SeedSeqFE128 / SeedSeqFE256). These types are not
//! engines, so they do NOT use EngineConformanceSuite.hpp; this is a small
//! dedicated suite covering the semantic [rand.req.seedseq] properties the
//! SeedSequence concept cannot check structurally: reproducibility, the
//! param()/generate() round-trip, distinctness, and that the sequence actually
//! drives a standard engine and an ni::random engine deterministically.
//!
//! Golden vectors are derived-then-pinned, exactly like the engines' golden
//! value: computed once from the real headers and frozen below. The Golden*
//! tests regenerate them and fail loudly on any algorithm or seeding drift.

#include <array>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/SeedSeqFE.hpp"
#include "ni/random/SeedSequence.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

namespace ni::random::test {

//! (1) The concept witnesses must compile inside this TU too — cheap, and it
//! documents that these types are genuine SeedSequences (and that
//! std::seed_seq, the oracle, is as well). The header-side static_asserts
//! already pin this; restating here keeps the intent local to the runtime
//! suite.
static_assert(SeedSequence<SeedSeqFE128>);
static_assert(SeedSequence<SeedSeqFE256>);
static_assert(SeedSequence<std::seed_seq>);

//! (2) Reproducibility + golden vector. Two SeedSeqFE128 built from the same
//! input produce identical generate() output, and that output matches the
//! pinned golden vector (8 words from input {1,2,3,4}).
TEST(SeedSeqFE128, GenerateIsDeterministicAndMatchesGolden) {
  static constexpr std::array<std::uint32_t, 8> kGolden = {
      0xfd6dff8bu, 0xd4a801ecu, 0x18a0c9dcu, 0x0fc9d615u,
      0x6c8e0218u, 0xf49dc889u, 0xc64750abu, 0x018db3e1u};

  SeedSeqFE128 a{1u, 2u, 3u, 4u};
  SeedSeqFE128 b{1u, 2u, 3u, 4u};

  std::array<std::uint32_t, 8> out_a{};
  std::array<std::uint32_t, 8> out_b{};
  a.generate(out_a.begin(), out_a.end());
  b.generate(out_b.begin(), out_b.end());

  EXPECT_EQ(out_a, out_b)
      << "same input must yield identical generate() output";
  EXPECT_EQ(out_a, kGolden)
      << "generate() drifted from the pinned golden vector";
}

//! (2b) Same golden discipline for the 256-bit alias (8 words from a 5-element
//! input — fewer inputs than the entropy store, exercising the zero-fill path).
TEST(SeedSeqFE256, GenerateMatchesGolden) {
  static constexpr std::array<std::uint32_t, 8> kGolden = {
      0xdb237714u, 0x8cd2d696u, 0x6742e798u, 0x3571ba4bu,
      0x1d7fbc91u, 0x7020e1bfu, 0x4027b49du, 0xab325ddbu};

  SeedSeqFE256 sseq{10u, 20u, 30u, 40u, 50u};
  std::array<std::uint32_t, 8> out{};
  sseq.generate(out.begin(), out.end());

  EXPECT_EQ(out, kGolden);
}

//! (3) Distinctness: different seed inputs produce different generate() output
//! (a sanity check against a no-op / constant seed sequence).
TEST(SeedSeqFE128, DistinctInputsProduceDistinctOutput) {
  SeedSeqFE128 a{1u, 2u, 3u, 4u};
  SeedSeqFE128 b{1u, 2u, 3u, 5u}; // single-word change

  std::array<std::uint32_t, 8> out_a{};
  std::array<std::uint32_t, 8> out_b{};
  a.generate(out_a.begin(), out_a.end());
  b.generate(out_b.begin(), out_b.end());

  EXPECT_NE(out_a, out_b);

  // And a default-constructed sequence differs from a seeded one.
  SeedSeqFE128 def;
  std::array<std::uint32_t, 8> out_def{};
  def.generate(out_def.begin(), out_def.end());
  EXPECT_NE(out_a, out_def);
}

//! (4) param() round-trips the entropy: a SeedSeqFE128 freshly constructed from
//! the words param() emits reproduces the original's generate() output. This is
//! the [rand.req.seedseq] param/round-trip property the concept cannot check.
TEST(SeedSeqFE128, ParamRoundTrips) {
  SeedSeqFE128 original{1u, 2u, 3u, 4u};

  std::array<std::uint32_t, SeedSeqFE128::size()> entropy{};
  original.param(entropy.begin());

  SeedSeqFE128 reconstructed(entropy.begin(), entropy.end());

  std::array<std::uint32_t, 8> out_orig{};
  std::array<std::uint32_t, 8> out_recon{};
  original.generate(out_orig.begin(), out_orig.end());
  reconstructed.generate(out_recon.begin(), out_recon.end());

  EXPECT_EQ(out_orig, out_recon)
      << "a sequence rebuilt from param() must reproduce generate()";
}

//! (5) The practical payoff: a seed_seq_fe actually drives engines, both a
//! standard one (std::mt19937) and an ni::random one (Xoshiro256PlusPlus),
//! deterministically and distinctly from a default-seeded engine. Golden first
//! outputs are pinned.
TEST(SeedSeqFE128, DrivesStandardEngineDeterministically) {
  SeedSeqFE128 sseq{1u, 2u, 3u, 4u};
  std::mt19937 seeded(sseq);

  EXPECT_EQ(seeded(), 1280205598u);
  EXPECT_EQ(seeded(), 1892172726u);
  EXPECT_EQ(seeded(), 2694122854u);

  // Reproducible: a second engine seeded the same way matches a fresh run.
  SeedSeqFE128 sseq2{1u, 2u, 3u, 4u};
  std::mt19937 seeded2(sseq2);
  std::mt19937 def; // default-seeded
  EXPECT_NE(seeded2(), def()) << "seed_seq_fe seeding must differ from default";
}

TEST(SeedSeqFE128, DrivesNiRandomEngineDeterministically) {
  SeedSeqFE128 sseq{1u, 2u, 3u, 4u};
  Xoshiro256PlusPlus seeded(sseq);

  EXPECT_EQ(seeded(), 13549603486845966949ull);
  EXPECT_EQ(seeded(), 14646982081242035358ull);
  EXPECT_EQ(seeded(), 2558539391091204602ull);

  SeedSeqFE128 sseq2{1u, 2u, 3u, 4u};
  Xoshiro256PlusPlus seeded2(sseq2);
  Xoshiro256PlusPlus def;
  EXPECT_NE(seeded2(), def());
}

//! (6) Coarse bias smoke check only — NOT a statistical test suite. seed_seq_fe
//! exists to fix std::seed_seq's bias; the empirical statistical properties are
//! established by the design docs (linked in SeedSeqFE.hpp), not re-derived
//! here. This merely confirms generate() doesn't produce an all-equal or
//! all-zero degenerate block for a normal input.
TEST(SeedSeqFE128, GenerateIsNotDegenerate) {
  SeedSeqFE128 sseq{1u, 2u, 3u, 4u};
  std::array<std::uint32_t, 16> out{};
  sseq.generate(out.begin(), out.end());

  bool all_zero = true;
  bool all_equal = true;
  for (std::size_t i = 0; i < out.size(); ++i) {
    if (out[i] != 0u)
      all_zero = false;
    if (out[i] != out[0])
      all_equal = false;
  }
  EXPECT_FALSE(all_zero);
  EXPECT_FALSE(all_equal);
}

} // namespace ni::random::test
