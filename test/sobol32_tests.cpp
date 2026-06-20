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

//! \file sobol32_tests.cpp
//!
//! Instantiates the reusable engine conformance suite for ni::random::Sobol32
//! (a 1-dimensional Sobol generator under the scalar [rand.req.eng] surface).
//! The family-neutral traits macro fits: "seeding" a Sobol engine means
//! choosing a starting POINT INDEX, so sample_seed = 42 is point index 42, and
//! discard composes as the scalar coordinate stream.
//!
//! The golden value is derived-then-pinned (Sobol has no standardized golden):
//! the 10000th scalar draw of a default 1-D engine is the Gray-order radical
//! inverse coordinate of point 10000, i.e. XOR of V_k = 2^(32-k) over the set
//! bits of gray(10000) = 0x3498 -> 0x192C0000 = 422313984. The GoldenValue test
//! fails loudly on any drift in the direction numbers or the Gray-code walk.

#include <gtest/gtest.h>

#include "ni/random/Sobol32.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

NI_RANDOM_XOSHIRO_TRAITS(Sobol32, 422313984u);

INSTANTIATE_TYPED_TEST_SUITE_P(Sobol32, EngineConformance, ni::random::Sobol32);

} // namespace ni::random::test
