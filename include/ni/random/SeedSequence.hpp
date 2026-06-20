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

//! \file SeedSequence.hpp
//!
//! Defines ni::random::SeedSequence — a concept that structurally gates any
//! type claiming to model the [rand.req.seedseq] "seed sequence" requirements
//! from the C++ standard.
//!
//! This concept is the shared contract for the seed-sequence material in this
//! library: it is consumed by SeedableBy / RandomNumberEngine
//! (RandomNumberEngine.hpp), and it gates the improved seed sequences
//! (SeedSeqFE.hpp). It lives in its own header so seed-sequence translation
//! units can depend on the contract without pulling in the full engine concept
//! (and its <istream>/<ostream> cost).
//!
//! Scope: like RandomNumberEngine, this checks ONLY what is structurally
//! checkable at compile time — the presence and signatures of the required
//! members. The semantic [rand.req.seedseq] requirements (e.g. that param()
//! round-trips the entropy fed to generate()) are NOT and CANNOT be verified by
//! a concept; they are delegated to the runtime gtest suite.

#include <climits>
#include <concepts>
#include <cstdint>

namespace ni::random {

//! The SeedSequence named requirement [rand.req.seedseq] — fully structural.
//! A type S is a seed sequence if it has an unsigned integral result_type of
//! at least 32 bits, is default-constructible, and exposes size(),
//! generate(first, last) over a RandomAccessIterator range, and a const
//! param(out) over an OutputIterator.
template <typename S>
concept SeedSequence = std::unsigned_integral<typename S::result_type> &&
                       (sizeof(typename S::result_type) * CHAR_BIT >= 32) &&
                       std::default_initializable<S> &&
                       requires(S q, const S cq, std::uint_least32_t *p) {
                         { q.size() } -> std::integral;
                         q.generate(p, p); // RandomAccessIterator range
                         cq.param(p);      // OutputIterator
                       };

} // namespace ni::random
