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

//! \file pcg32_c64_fast_tests.cpp
//!
//! Instantiates the reusable conformance suite (EngineConformanceSuite.hpp) for
//! the PCG engine ni::random::Pcg32C64Fast. Per the suite's design an engine
//! needs only its traits specialization — here via NI_RANDOM_XOSHIRO_TRAITS,
//! the family-neutral traits macro (its fields fit PCG: a single std::uint64_t
//! sample_seed converts to both the 64-bit and 128-bit PCG seed types) — plus
//! the single INSTANTIATE_TYPED_TEST_SUITE_P line; the generic test bodies are
//! shared and untouched. The golden value is the 10000th output of a
//! default-constructed engine, derived once and frozen here; the GoldenValue
//! test regenerates it and fails on any algorithm or seeding drift.

#include <gtest/gtest.h>

#include "ni/random/Pcg32C64Fast.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

NI_RANDOM_XOSHIRO_TRAITS(Pcg32C64Fast, 1391258508ull);

INSTANTIATE_TYPED_TEST_SUITE_P(Pcg32C64Fast, EngineConformance,
                               ni::random::Pcg32C64Fast);

} // namespace ni::random::test
