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

//! \file chacha8_tests.cpp
//!
//! Instantiates the reusable conformance suite (EngineConformanceSuite.hpp) for
//! the reduced-round ni::random::ChaCha8 on its NATIVE block policy (NEON on
//! ARM, SSE on x86, scalar elsewhere). ChaCha8 shares ChaCha20's interface,
//! block policies, and seeding; only the round count (and therefore the golden
//! value) differs, so it needs only its traits specialization plus the single
//! INSTANTIATE_TYPED_TEST_SUITE_P line.
//!
//! The golden value is derived-then-pinned: the 10000th output of a
//! default-constructed ChaCha8 (seed 1234567890, stream 0). The forced-scalar
//! anchor (chacha8_scalar_tests.cpp) pins the SAME value, so a divergence there
//! localizes the bug to a SIMD block.

#include <gtest/gtest.h>

#include "ni/random/ChaCha.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

NI_RANDOM_XOSHIRO_TRAITS(ChaCha8, 238947013u);

INSTANTIATE_TYPED_TEST_SUITE_P(ChaCha8, EngineConformance, ni::random::ChaCha8);

} // namespace ni::random::test
