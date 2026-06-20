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

//! \file seed_pod_tests.cpp
//!
//! Behavioral tests for ni::random::SeedPod: NewSeed() is strictly increasing
//! and never zero, and ClaimSeed advances the sequence past the claimed value
//! (and is a no-op for values already in the past).

#include <cstdint>

#include <gtest/gtest.h>

#include "ni/random/SeedPod.hpp"

namespace ni::random::test {
namespace {

TEST(SeedPod, NewSeedIsStrictlyIncreasing) {
  SeedPod<std::uint64_t> pod;
  auto previous = pod.NewSeed();
  for (int i = 0; i < 1000; ++i) {
    const auto next = pod.NewSeed();
    EXPECT_GT(next, previous);
    previous = next;
  }
}

TEST(SeedPod, NewSeedNeverReturnsZero) {
  SeedPod<std::uint64_t> pod;
  for (int i = 0; i < 1000; ++i) {
    EXPECT_NE(pod.NewSeed(), 0u);
  }
}

TEST(SeedPod, ClaimSeedAdvancesSequence) {
  SeedPod<std::uint64_t> pod;
  const std::uint64_t claimed = pod.NewSeed() + 100000u;
  pod.ClaimSeed(claimed);
  EXPECT_GT(pod.NewSeed(), claimed);
}

TEST(SeedPod, ClaimSeedInThePastIsNoOp) {
  SeedPod<std::uint64_t> pod;
  const auto current = pod.NewSeed();
  pod.ClaimSeed(current > 0u ? current - 1u : 0u);
  EXPECT_GT(pod.NewSeed(), current);
}

} // namespace
} // namespace ni::random::test
