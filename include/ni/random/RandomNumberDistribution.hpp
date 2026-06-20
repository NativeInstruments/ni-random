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

//! \file RandomNumberDistribution.hpp
//!
//! Defines ni::random::RandomNumberDistribution — a concept that structurally
//! gates any type claiming to model the [rand.req.dist] "random number
//! distribution" requirements from the C++ standard. It is the distribution
//! analogue of RandomNumberEngine and follows the same discipline.
//!
//! Scope and honest limitations:
//!
//! - This concept checks ONLY what is structurally checkable at compile time:
//!   the presence and signatures of the required members and free operators.
//!   The *semantic* requirements — reproducibility (equal distributions driven
//!   by equal engines produce equal sequences), param round-trip, reset
//!   behavior, and stream round-trip identity — are NOT and CANNOT be verified
//!   by a concept. Those are delegated to the runtime, type-parametrized gtest
//!   suite that accompanies this header (DistributionConformanceSuite.hpp).
//!
//! - A distribution does NOT refine std::uniform_random_bit_generator; its
//!   result_type is the sampled value type (a signed int, double, float, ...),
//!   so the concept requires only that result_type is *arithmetic* — never
//!   unsigned-integral.
//!
//! - The operator() and stream clauses are NECESSARILY witness-based. A concept
//!   cannot universally quantify over "all uniform random bit generators" or
//!   "all charT", so each clause pins one canonical witness: std::minstd_rand
//!   for the generator (a std engine, so this header gains no dependency on a
//!   ni engine), and char streams for I/O. Verifying that a distribution also
//!   works with a *ni* engine and other char widths is the suite's job.
//!
//! - Oracle vs. golden: the std::*_distribution types below are the
//!   known-conforming oracle that proves this concept is satisfiable / not
//!   over-constrained. They are an oracle for the CONCEPT only — std
//!   distributions are NOT specified to be reproducible across
//!   implementations, so they cannot pin a golden value. Golden-value
//!   conformance is verified on the (platform-reproducible) UTL distributions
//!   by the suite.

#include <concepts>
#include <istream>
#include <ostream>
#include <random>
#include <type_traits>

namespace ni::random {

//! "D is invocable as a distribution driven by URBG type G" — G is the explicit
//! witness (std::minstd_rand below). Forces both operator() overloads.
template <typename D, typename G>
concept GeneratesWith = std::uniform_random_bit_generator<G> &&
                        requires(D d, const typename D::param_type &p, G &g) {
                          { d(g) } -> std::same_as<typename D::result_type>;
                          { d(g, p) } -> std::same_as<typename D::result_type>;
                        };

//! "D inserts/extracts over basic_ostream/istream<CharT, Traits>" — CharT is
//! the explicit witness (char below). operator<< takes the distribution by
//! const&.
template <typename D, typename CharT, typename Traits>
concept DistributionStreamSerializable =
    requires(std::basic_ostream<CharT, Traits> &os, const D &cd,
             std::basic_istream<CharT, Traits> &is, D &d) {
      { os << cd } -> std::same_as<std::basic_ostream<CharT, Traits> &>;
      { is >> d } -> std::same_as<std::basic_istream<CharT, Traits> &>;
    };

//! RandomNumberDistribution models the [rand.req.dist] table structurally.
//! Every clause maps to a row of that table; semantics (reproducibility, param
//! round-trip, reset, stream round-trip, golden value) are checked at runtime
//! by the suite.
template <typename D>
concept RandomNumberDistribution =
    std::is_arithmetic_v<typename D::result_type> &&
    std::default_initializable<D> && std::copyable<D> &&
    std::equality_comparable<D> && requires { typename D::param_type; } &&
    std::same_as<typename D::param_type::distribution_type, D> &&
    std::default_initializable<typename D::param_type> &&
    std::copyable<typename D::param_type> &&
    std::equality_comparable<typename D::param_type> &&
    requires(D d, const D cd, const typename D::param_type &p) {
      D(p);
      { cd.param() } -> std::same_as<typename D::param_type>;
      { d.param(p) } -> std::same_as<void>;
      d.reset();
      { cd.min() } -> std::same_as<typename D::result_type>;
      { cd.max() } -> std::same_as<typename D::result_type>;
    } && GeneratesWith<D, std::minstd_rand> &&
    DistributionStreamSerializable<D, char, std::char_traits<char>>;

//! Build-time smoke checks against known-conforming std distributions — the
//! oracle that proves the concept is satisfiable / not over-constrained. (These
//! std types are NOT reproducible across implementations, so they are an oracle
//! for the CONCEPT only; golden-value conformance is verified on the UTL
//! distributions by the suite.)
static_assert(RandomNumberDistribution<std::uniform_int_distribution<int>>);
static_assert(RandomNumberDistribution<std::uniform_real_distribution<double>>);
static_assert(RandomNumberDistribution<std::normal_distribution<double>>);

} // namespace ni::random
