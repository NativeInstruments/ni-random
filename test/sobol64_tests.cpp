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

//! \file sobol64_tests.cpp
//!
//! Instantiates the reusable engine conformance suite for ni::random::Sobol64
//! (the 64-bit-width Sobol generator). See sobol32_tests.cpp for the seeding /
//! discard semantics. The golden value is the 64-bit analogue of the Sobol32
//! one: the 10000th scalar draw is XOR of V_k = 2^(64-k) over the set bits of
//! gray(10000) = 0x3498, i.e. the Sobol32 pattern shifted into the top 64 bits:
//! 0x192C000000000000 = 1813824749923467264. Derived-then-pinned.

#include <gtest/gtest.h>

#include "ni/random/Sobol64.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

NI_RANDOM_XOSHIRO_TRAITS(Sobol64, 1813824749923467264ull);

INSTANTIATE_TYPED_TEST_SUITE_P(Sobol64, EngineConformance, ni::random::Sobol64);

} // namespace ni::random::test
