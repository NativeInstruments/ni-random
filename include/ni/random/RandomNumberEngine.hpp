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

//! \file RandomNumberEngine.hpp
//!
//! Defines ni::random::RandomNumberEngine — a concept that structurally gates
//! any type claiming to model the [rand.req.eng] "random number engine"
//! requirements from the C++ standard.
//!
//! Scope and honest limitations:
//!
//! - This concept checks ONLY what is structurally checkable at compile time:
//!   the presence and signatures of the required members and free operators.
//!   The *semantic* requirements — reproducibility (equal engines produce equal
//!   sequences), discard(z) being equivalent to z invocations, and stream
//!   round-trip identity — are NOT and CANNOT be verified by a concept. Those
//!   are delegated to the runtime, type-parametrized gtest suite that
//!   accompanies this header (EngineConformanceSuite.hpp).
//!
//! - The seed-sequence and stream clauses are NECESSARILY witness-based. A
//!   concept cannot universally quantify over "all SeedSequence types" or "all
//!   charT", so each clause pins one canonical witness: std::seed_seq for
//!   seed-sequence construction/seeding, and char streams for I/O. Verifying
//!   that an engine also works with *other* seed-sequence / stream types is the
//!   suite's job (it folds over additional adversarial witnesses). The helper
//!   concepts below (SeedSequence, SeedableBy, StreamSerializable) make the
//!   witnesses explicit and reusable so the suite can probe more of them; they
//!   relocate the witness, they do not remove it.
//!
//! - Stream-clause dependency tradeoff (decided deliberately): including
//!   StreamSerializable forces <istream>/<ostream> (locale/facet machinery)
//!   into this header, so every TU that merely *constrains* on
//!   RandomNumberEngine pays for iostreams. For this library we include it for
//!   completeness. REJECTED ALTERNATIVE: if a freestanding / lightweight
//!   compile-time constraint is later needed, drop the stream clause from the
//!   concept and verify round-trip only in the suite (test #11).

#include "SeedSequence.hpp"

#include <concepts>
#include <istream>
#include <ostream>
#include <random>

namespace ni::random {

//! "E is seedable by seed-sequence type Sseq" — Sseq is the explicit witness.
//! The Sseq& lvalue parameter is what selects the seed-sequence overload of
//! the constructor and seed() (std::seed_seq is non-copyable, so the standard
//! overloads take an lvalue reference).
template <typename E, typename Sseq>
concept SeedableBy = SeedSequence<Sseq> && requires(E e, Sseq &q) {
  E(q);
  { e.seed(q) } -> std::same_as<void>;
};

//! "E inserts/extracts over basic_ostream/istream<CharT, Traits>" — CharT is
//! the explicit witness. operator<< takes the engine by const&.
template <typename E, typename CharT, typename Traits>
concept StreamSerializable =
    requires(std::basic_ostream<CharT, Traits> &os, const E &ce,
             std::basic_istream<CharT, Traits> &is, E &e) {
      { os << ce } -> std::same_as<std::basic_ostream<CharT, Traits> &>;
      { is >> e } -> std::same_as<std::basic_istream<CharT, Traits> &>;
    };

//! RandomNumberEngine refines std::uniform_random_bit_generator with the
//! structural members the [rand.req.eng] table mandates. Every clause maps to
//! a row of that table; semantics are checked at runtime by the suite.
template <typename E>
concept RandomNumberEngine =
    std::uniform_random_bit_generator<E> && std::default_initializable<E> &&
    std::copyable<E> && std::equality_comparable<E> &&
    std::unsigned_integral<typename E::result_type> &&
    requires(E e, typename E::result_type s, unsigned long long z) {
      E(s);
      { e.seed(s) } -> std::same_as<void>;
      { e.seed() } -> std::same_as<void>;
      { e.discard(z) } -> std::same_as<void>;
    } && SeedableBy<E, std::seed_seq> &&
    StreamSerializable<E, char, std::char_traits<char>>;

//! Build-time smoke check against the reference engine. std::minstd_rand is
//! the known-conforming oracle for this whole library.
static_assert(RandomNumberEngine<std::minstd_rand>);

//! std::seed_seq is the known-conforming SeedSequence oracle. Gating it here,
//! beside the engine oracle, anchors the seed-sequence contract the same way.
static_assert(SeedSequence<std::seed_seq>);

} // namespace ni::random
