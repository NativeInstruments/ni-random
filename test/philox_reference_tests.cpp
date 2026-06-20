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

//! \file philox_reference_tests.cpp
//!
//! Round-function known-answer test. Pins the first Philox(K, X) batch (counter
//! 0, the default-seed key) for each predefined engine, catching round-function
//! errors independently of the seeding/batching path the GoldenValue test
//! (10000th invocation) exercises. The expected values are derived from the
//! C++26 [rand.eng.philox] algorithm: captured from the engine once its
//! GoldenValue test confirmed it reproduces the standard's mandated 10000th
//! value -- i.e. from a known-good run, not from any third-party (random123)
//! source. A mismatch means the round function regressed; never repin.

#include <array>
#include <cstddef>

#include <gtest/gtest.h>

#include "ni/random/Philox4x32.hpp"
#include "ni/random/Philox4x64.hpp"

namespace ni::random::test {

//! First four outputs of a default-constructed Philox4x32 -- the Philox(K, X)
//! batch for counter 0 with K0 = default_seed.
TEST(PhiloxReference, Philox4x32FirstBatch) {
  ni::random::Philox4x32 e;
  const std::array<ni::random::Philox4x32::result_type, 4> expected = {
      3587538684u, 1324224816u, 3068087177u, 2030706281u};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(e(), expected[i]) << "element " << i;
  }
}

//! First four outputs of a default-constructed Philox4x64 -- the Philox(K, X)
//! batch for counter 0 with K0 = default_seed.
TEST(PhiloxReference, Philox4x64FirstBatch) {
  ni::random::Philox4x64 e;
  const std::array<ni::random::Philox4x64::result_type, 4> expected = {
      4854577551194240716ull, 11024447680751626801ull, 6491473261962256061ull,
      17735969495851009945ull};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(e(), expected[i]) << "element " << i;
  }
}

} // namespace ni::random::test
