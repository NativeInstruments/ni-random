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

//! \file approx_normal_distribution_tests.cpp
//!
//! Instantiates the reusable distribution conformance suite
//! (DistributionConformanceSuite.hpp) for
//! ni::random::ApproxNormalDistribution<double>.
//!
//! ApproxNormalDistribution is NON-STANDARD (a fast, imprecise normal
//! approximation) so there is no std oracle for it; it still must satisfy
//! RandomNumberDistribution. It requires a bit-uniform 32/64-bit witness engine
//! (approx_standard_normal static_asserts this); Xoshiro256PlusPlus is a
//! bit-uniform 64-bit engine, so it qualifies.

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "ni/random/ApproxNormalDistribution.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

#include "DistributionConformanceSuite.hpp"
#include "DistributionConformanceTraits.hpp"

namespace ni::random::test {

template <>
struct DistributionConformanceTraits<
    ni::random::ApproxNormalDistribution<double>> {
  using distribution_type = ni::random::ApproxNormalDistribution<double>;
  using witness_engine = ni::random::Xoshiro256PlusPlus;
  static constexpr std::uint64_t engine_seed = 42u;
  static constexpr distribution_type::param_type sample_param{3.0, 2.0};
  static constexpr distribution_type::param_type other_param{0.0, 1.0};
  static constexpr std::size_t golden_n = 10000;
  //! Derived-then-pinned: the 10000th draw of a default
  //! ApproxNormalDistribution driven by a default Xoshiro256PlusPlus. The draw
  //! is deterministic, so the suite compares it with exact EXPECT_EQ.
  static constexpr distribution_type::result_type golden_value =
      -1.779331521535221;
};

INSTANTIATE_TYPED_TEST_SUITE_P(ApproxNormal, DistributionConformance,
                               ni::random::ApproxNormalDistribution<double>);

} // namespace ni::random::test
