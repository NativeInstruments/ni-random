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

//! \file DistributionConformanceTraits.hpp
//!
//! Shared scaffolding for the per-distribution configuration the
//! type-parametrized conformance suite reads. The generic test bodies
//! (DistributionConformanceSuite.hpp) pull everything distribution-specific
//! from DistributionConformanceTraits<D>, so the bodies never hardcode a param,
//! a draw count, or a golden value.
//!
//! Unlike the engine traits, there is no family macro: the four distributions
//! differ enough (int vs. real params; stateful vs. stateless) that each gets a
//! hand-written specialization beside its INSTANTIATE_TYPED_TEST_SUITE_P line
//! in its own *_tests.cpp. Each specialization provides:
//!
//!   using distribution_type = D;
//!   using witness_engine    = ni::random::Xoshiro256PlusPlus;  // drives every
//!   test static constexpr std::uint64_t engine_seed  = ...;         // seed
//!   for the witness static constexpr D::param_type sample_param = ...; // a
//!   non-default param static constexpr D::param_type other_param  = ...; //
//!   distinct from sample_param static constexpr std::size_t   golden_n     =
//!   10000; static constexpr D::result_type golden_value = ...;        //
//!   derived-then-pinned
//!
//! golden_value is the golden_n-th draw of a DEFAULT distribution driven by a
//! DEFAULT-constructed witness_engine, computed once and frozen so the
//! GoldenValue test fails loudly on any drift. For floating-point result_types
//! the draw is deterministic (not approximate), so the suite compares it with
//! EXPECT_EQ — exact bit equality, no tolerance.

#include <cstddef>
#include <cstdint>

namespace ni::random::test {

//! Primary template is intentionally undefined: instantiating the suite for a
//! distribution that lacks a traits specialization is a compile error, not a
//! silent default.
template <typename D> struct DistributionConformanceTraits;

} // namespace ni::random::test
