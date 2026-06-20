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

//! \file simple_uniform_int_distribution_tests.cpp
//!
//! Instantiates the reusable distribution conformance suite
//! (DistributionConformanceSuite.hpp) for
//! ni::random::SimpleUniformIntDistribution<unsigned>. The traits
//! specialization lives beside the INSTANTIATE_TYPED_TEST_SUITE_P line,
//! following the engine-suite pattern; the generic test bodies are shared and
//! untouched.
//!
//! sample_param/other_param keep a >= 1: this distribution maps [a - 0.5,
//! b + 0.5) onto the integers, so an a of 0 would let the bottom edge round to
//! -1 and underflow the unsigned result_type. The conformance suite only ever
//! draws with sample_param, so keeping its lower bound >= 1 exercises the
//! in-range behavior without tripping that edge.

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "ni/random/SimpleUniformIntDistribution.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

#include "DistributionConformanceSuite.hpp"
#include "DistributionConformanceTraits.hpp"

namespace ni::random::test {

template <>
struct DistributionConformanceTraits<
    ni::random::SimpleUniformIntDistribution<unsigned>> {
  using distribution_type = ni::random::SimpleUniformIntDistribution<unsigned>;
  using witness_engine = ni::random::Xoshiro256PlusPlus;
  static constexpr std::uint64_t engine_seed = 42u;
  static constexpr distribution_type::param_type sample_param{1u, 6u};
  static constexpr distribution_type::param_type other_param{2u, 9u};
  static constexpr std::size_t golden_n = 10000;
  //! Derived-then-pinned: the 10000th draw of a default
  //! SimpleUniformIntDistribution ([0, UINT_MAX]) driven by a default
  //! Xoshiro256PlusPlus.
  static constexpr distribution_type::result_type golden_value = 3025191424u;
};

INSTANTIATE_TYPED_TEST_SUITE_P(
    SimpleUniformInt, DistributionConformance,
    ni::random::SimpleUniformIntDistribution<unsigned>);

} // namespace ni::random::test
