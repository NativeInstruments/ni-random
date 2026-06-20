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

//! \file chacha_substream_tests.cpp
//!
//! Covers the ChaCha `stream` substream parameter — kept as public API
//! (ChaCha(seed, stream) / seed(seed, stream)) and the ChaCha analogue of the
//! xoshiro family's jump()/longJump() and Philox's set_counter. The generic
//! EngineConformanceSuite never passes a non-zero stream, so this dedicated
//! file (in the spirit of philox_set_counter_tests.cpp) pins the substream
//! contract: same seed + different stream → different, non-lockstep output;
//! same seed + same stream → equal engines that stay in lockstep.

#include <cstdint>

#include <gtest/gtest.h>

#include "ni/random/ChaCha.hpp"

namespace ni::random::test {

//! Two engines with the same seed but different stream values diverge — the
//! independent-substream property the stream parameter exists for.
TEST(ChaChaSubstream, DistinctStreamsDiffer) {
  constexpr std::uint64_t seed = 1234567890ull;
  ni::random::ChaCha20 a(seed, 0);
  ni::random::ChaCha20 b(seed, 1);

  EXPECT_NE(a, b);
  bool anyDifferent = false;
  for (int i = 0; i < 1000; ++i) {
    if (a() != b()) {
      anyDifferent = true;
    }
  }
  EXPECT_TRUE(anyDifferent);
}

//! Far-apart streams from the same seed also diverge — guards against a stream
//! value that only touches low bits of the key setup.
TEST(ChaChaSubstream, FarApartStreamsDiffer) {
  constexpr std::uint64_t seed = 42ull;
  ni::random::ChaCha20 a(seed, 7ull);
  ni::random::ChaCha20 b(seed, 0xABCDEF0123456789ull);

  bool anyDifferent = false;
  for (int i = 0; i < 1000; ++i) {
    if (a() != b()) {
      anyDifferent = true;
    }
  }
  EXPECT_TRUE(anyDifferent);
}

//! Same seed AND same (non-zero) stream → equal engines that stay in lockstep,
//! whether the stream is set via the ctor or via seed(seed, stream).
TEST(ChaChaSubstream, SameSeedAndStreamLockstep) {
  constexpr std::uint64_t seed = 0xDEADBEEFull;
  constexpr std::uint64_t stream = 99ull;
  ni::random::ChaCha20 a(seed, stream);
  ni::random::ChaCha20 b;
  b.seed(seed, stream);

  EXPECT_EQ(a, b);
  for (int i = 0; i < 1000; ++i) {
    EXPECT_EQ(a(), b());
  }
}

} // namespace ni::random::test
