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

//! \file pcg64_emulated128_tests.cpp
//!
//! Runs the standard conformance suite for the core 128-bit PCG engines against
//! the SOFTWARE 128-bit math path (pcg_extras::uint_x4), the path that actually
//! ships on MSVC where __uint128_t is unavailable. The dev machine and the
//! macOS xctest run both have native __uint128_t, so without this TU the
//! emulated math would be exercised only in Windows CI, if at all.
//!
//! PCG's native and emulated 128-bit paths are required to produce identical
//! sequences, so each engine here pins the SAME golden value as its native
//! sibling (pcg64_tests.cpp etc.). A mismatch is a real bug in the emulated
//! math — exactly what this test exists to catch.
//!
//! Macro-ordering hazard: PCG_FORCE_EMULATED_128BIT_MATH changes the definition
//! of pcg128_t, so it MUST be defined before the first ni-random include. That
//! makes ni::random::Pcg64 here a genuinely different type (uint_x4-based) than
//! the native-path Pcg64 in pcg64_tests.cpp — hence the EngineConformanceTraits
//! specializations below do not collide with the native ones, and the suite is
//! instantiated under a distinct Pcg64Emulated* prefix.
//!
//! Scope note: only the CORE pcg64 family is covered. The extended k/c 128-bit
//! engines (Pcg64K32*/Pcg64C32*) do NOT compile on the emulated path — their
//! extended<> table_mask is a constexpr that uint_x4 cannot initialize at
//! compile time — so they are deliberately excluded here (and are a known
//! MSVC-viability risk flagged in progress/import-pcg-family.md).

#define PCG_FORCE_EMULATED_128BIT_MATH 1

#include <gtest/gtest.h>

#include "ni/random/Pcg64.hpp"
#include "ni/random/Pcg64Fast.hpp"
#include "ni/random/Pcg64OneSeq.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

NI_RANDOM_XOSHIRO_TRAITS(Pcg64, 11135645891219275043ull);
NI_RANDOM_XOSHIRO_TRAITS(Pcg64OneSeq, 11135645891219275043ull);
NI_RANDOM_XOSHIRO_TRAITS(Pcg64Fast, 10120502117314785475ull);

INSTANTIATE_TYPED_TEST_SUITE_P(Pcg64Emulated, EngineConformance,
                               ni::random::Pcg64);
INSTANTIATE_TYPED_TEST_SUITE_P(Pcg64OneSeqEmulated, EngineConformance,
                               ni::random::Pcg64OneSeq);
INSTANTIATE_TYPED_TEST_SUITE_P(Pcg64FastEmulated, EngineConformance,
                               ni::random::Pcg64Fast);

} // namespace ni::random::test
