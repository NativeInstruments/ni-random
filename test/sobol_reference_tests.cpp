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

//! \file sobol_reference_tests.cpp
//!
//! Known-answer tests for the Sobol generator, independent of the generic
//! conformance suite. They pin:
//!   1. the direction-number recurrence (V values for dimensions 1-3, W=32),
//!   2. the 1-D Gray-order radical-inverse coordinates,
//!   3. the first several 2-D and 3-D points.
//!
//! Reference values are computed from the embedded direction numbers
//! (new-joe-kuo-6.21201, dims 2-3: s=1,a=0,m=1 and s=2,a=1,m=1,3) via the
//! Antonov-Saleev / Gray-code construction. They are the canonical Sobol points
//! and agree with scipy.stats.qmc.Sobol(scramble=False) and Boost.Random's
//! sobol (Gray order; the origin is skipped, so the first emitted point is
//! index 1). Source: Joe & Kuo, https://web.maths.unsw.edu.au/~fkuo/sobol/.

#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/Sobol32.hpp"
#include "ni/random/detail/SobolDirectionNumbers.hpp"

namespace ni::random::test {

//! Direction integers V_{k,j} for the first three dimensions, hand-derived from
//! the Joe-Kuo polynomials, confirm the V-space recurrence.
TEST(SobolReference, DirectionNumbers) {
  const auto v = detail::buildSobolDirectionNumbers<std::uint32_t, 32>(
      3, detail::defaultSobolDirectionTable());

  // Dimension 1 (radical inverse): V_k = 2^(32-k).
  EXPECT_EQ(v[0][0], 0x80000000u);
  EXPECT_EQ(v[0][1], 0x40000000u);
  EXPECT_EQ(v[0][2], 0x20000000u);

  // Dimension 2: s=1, a=0, m_1=1 -> V_k = V_{k-1} ^ (V_{k-1} >> 1).
  EXPECT_EQ(v[1][0], 0x80000000u);
  EXPECT_EQ(v[1][1], 0xC0000000u);
  EXPECT_EQ(v[1][2], 0xA0000000u);

  // Dimension 3: s=2, a=1, m_1=1, m_2=3.
  EXPECT_EQ(v[2][0], 0x80000000u);
  EXPECT_EQ(v[2][1], 0xC0000000u);
  EXPECT_EQ(v[2][2], 0x60000000u);
  EXPECT_EQ(v[2][3], 0x90000000u);
}

//! 1-D Gray-order sequence (the Antonov-Saleev permutation of the van der
//! Corput / radical-inverse sequence). NOTE: this is 0.5, 0.75, 0.25, ... in
//! Gray order, NOT the natural 0.5, 0.25, 0.75 order -- they are the same point
//! set, and Gray order is what every reference Sobol generator emits.
TEST(SobolReference, OneDimensionalRadicalInverse) {
  ni::random::Sobol32 e;
  ASSERT_EQ(e.dimension(), 1u);
  const std::array<double, 7> expected = {0.5,   0.75,  0.25, 0.375,
                                          0.875, 0.625, 0.125};
  for (double want : expected) {
    const auto point = e.next_point();
    ASSERT_EQ(point.size(), 1u);
    EXPECT_DOUBLE_EQ(point[0], want);
  }
}

//! First seven 2-D points (canonical Sobol; matches scipy/Boost).
TEST(SobolReference, TwoDimensionalPoints) {
  ni::random::Sobol32 e{ni::random::dimensions(2)};
  const std::array<std::array<double, 2>, 7> expected = {{
      {0.5, 0.5},
      {0.75, 0.25},
      {0.25, 0.75},
      {0.375, 0.375},
      {0.875, 0.875},
      {0.625, 0.125},
      {0.125, 0.625},
  }};
  for (const auto &want : expected) {
    const auto point = e.next_point();
    ASSERT_EQ(point.size(), 2u);
    EXPECT_DOUBLE_EQ(point[0], want[0]);
    EXPECT_DOUBLE_EQ(point[1], want[1]);
  }
}

//! First seven 3-D points.
TEST(SobolReference, ThreeDimensionalPoints) {
  ni::random::Sobol32 e{ni::random::dimensions(3)};
  const std::array<std::array<double, 3>, 7> expected = {{
      {0.5, 0.5, 0.5},
      {0.75, 0.25, 0.25},
      {0.25, 0.75, 0.75},
      {0.375, 0.375, 0.625},
      {0.875, 0.875, 0.125},
      {0.625, 0.125, 0.875},
      {0.125, 0.625, 0.375},
  }};
  for (const auto &want : expected) {
    const auto point = e.next_point();
    ASSERT_EQ(point.size(), 3u);
    EXPECT_DOUBLE_EQ(point[0], want[0]);
    EXPECT_DOUBLE_EQ(point[1], want[1]);
    EXPECT_DOUBLE_EQ(point[2], want[2]);
  }
}

} // namespace ni::random::test
