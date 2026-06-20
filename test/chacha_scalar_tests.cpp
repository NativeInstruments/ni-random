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

//! \file chacha_scalar_tests.cpp
//!
//! Runs the full conformance suite against the FORCED-SCALAR ChaCha20 — the
//! engine pinned to ni::random::detail::ScalarBlock — so the portable scalar
//! path is golden/discard/seed-seq/stream-checked on EVERY platform, not just
//! where it happens to be DefaultBlock. On the dev machine (Apple Silicon) and
//! on x86 CI the native path is SIMD, so without this TU the scalar reference
//! that the SIMD paths are required to reproduce would never itself be run
//! through the suite.
//!
//! The traits macro NI_RANDOM_XOSHIRO_TRAITS hardcodes the ni::random::
//! namespace and so cannot name a detail:: type; the specialization below is
//! therefore hand-written (mirroring minstd_rand_tests.cpp) for the
//! scalar-policy type, and instantiated under a distinct ChaCha20Scalar prefix
//! so it does not collide with the native instantiation in chacha20_tests.cpp.
//!
//! It pins the SAME golden value as the native suite (2886286512): the scalar
//! and SIMD paths are required to be bit-identical, so a mismatch here is a
//! real bug in a SIMD block — never repin to paper over a SIMD divergence.

#include <cstddef>
#include <cstdint>
#include <tuple>

#include <gtest/gtest.h>

#include "ni/random/ChaCha.hpp"

#include "EngineConformanceSuite.hpp"
#include "EngineConformanceTraits.hpp"

namespace ni::random::test {

using ChaCha20Scalar = ni::random::detail::ChaChaImpl<20, detail::ScalarBlock>;

//! Hand-written traits for the forced-scalar ChaCha20. Identical to what the
//! family macro would emit, but spelled out because the macro cannot name a
//! detail:: type. Same golden value as the native ChaCha20 — the two paths must
//! agree.
template <> struct EngineConformanceTraits<ChaCha20Scalar> {
  using engine_type = ChaCha20Scalar;
  static constexpr unsigned long long discard_counts[] = {0ull, 1ull, 7ull,
                                                          100000ull};
  static constexpr std::size_t golden_n = 10000;
  static constexpr ChaCha20Scalar::result_type golden_value = 2886286512u;
  static constexpr std::uint64_t sample_seed = 42u;
  using extra_seed_seqs = std::tuple<>;
};

INSTANTIATE_TYPED_TEST_SUITE_P(ChaCha20Scalar, EngineConformance,
                               ChaCha20Scalar);

} // namespace ni::random::test
