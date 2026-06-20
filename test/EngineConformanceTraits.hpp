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

#pragma once

//! \file EngineConformanceTraits.hpp
//!
//! Shared scaffolding for the per-engine configuration the type-parametrized
//! conformance suite reads. The generic test bodies
//! (EngineConformanceSuite.hpp) pull everything engine-specific from
//! EngineConformanceTraits<E>, so the bodies never hardcode a seed, a draw
//! count, or a golden value.
//!
//! This header holds only the primary template and the family macro. Each
//! engine's traits specialization lives next to that engine's
//! INSTANTIATE_TYPED_TEST_SUITE_P line in its own *_tests.cpp — for the
//! xoshiro/xoroshiro family via NI_RANDOM_XOSHIRO_TRAITS below, and for
//! std::minstd_rand hand-written in minstd_rand_tests.cpp.

#include <cstddef>
#include <cstdint>
#include <tuple>

namespace ni::random::test {

//! Primary template is intentionally undefined: instantiating the suite for
//! an engine that lacks a traits specialization is a compile error, not a
//! silent default.
template <typename E> struct EngineConformanceTraits;

//! Defines the traits specialization for one xoshiro/xoroshiro family engine.
//! Every family engine shares the same discard_counts, golden_n, and
//! sample_seed; they differ only in the pinned golden_value, so this macro
//! keeps the ten specializations honest and in lockstep. Invoke it once, inside
//! namespace ni::random::test, in the engine's *_tests.cpp (after including the
//! engine header so the type is complete).
//!
//! Each golden_value is *derived then pinned*: it is the golden_n-th output of
//! a default-constructed engine, computed once and frozen at the call site so
//! the GoldenValue test fails loudly on any drift in the algorithm or its
//! seeding. sample_seed is a std::uint64_t to match every family ctor's seed
//! parameter (the 32-bit-output engines still seed from a 64-bit value via
//! SplitMix64).
#define NI_RANDOM_XOSHIRO_TRAITS(Engine, Golden)                               \
  template <> struct EngineConformanceTraits<ni::random::Engine> {             \
    using engine_type = ni::random::Engine;                                    \
    static constexpr unsigned long long discard_counts[] = {0ull, 1ull, 7ull,  \
                                                            100000ull};        \
    static constexpr std::size_t golden_n = 10000;                             \
    static constexpr ni::random::Engine::result_type golden_value = Golden;    \
    static constexpr std::uint64_t sample_seed = 42u;                          \
    using extra_seed_seqs = std::tuple<>;                                      \
  }

} // namespace ni::random::test
