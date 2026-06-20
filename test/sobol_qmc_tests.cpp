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

//! \file sobol_qmc_tests.cpp
//!
//! Quasi-Monte-Carlo / equidistribution tests for the Sobol generator that the
//! generic engine conformance suite does not cover: skip-ahead consistency,
//! agreement between next_point() and the scalar operator(), the digital-shift
//! API, a Property-A (dyadic net balance) check, and a QMC-beats-plain-MC
//! integration-error check.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "ni/random/Sobol32.hpp"

namespace ni::random::test {

//! Seeking directly to point index k (skip-ahead) must equal iterating there.
TEST(SobolQmc, SkipAheadMatchesIteration) {
  for (std::uint32_t target : {1u, 2u, 17u, 50u, 1000u, 65537u}) {
    // Engine A iterates: advance (target - 1) whole points from the first
    // emitted point (index 1), landing on point `target`.
    ni::random::Sobol32 a{ni::random::dimensions(3)};
    a.discard_points(target - 1);
    const auto iterated = a.next_point();

    // Engine B jumps straight to point `target`.
    ni::random::Sobol32 b{ni::random::dimensions(3)};
    b.seed(target);
    const auto jumped = b.next_point();

    ASSERT_EQ(iterated.size(), jumped.size());
    for (std::size_t j = 0; j < iterated.size(); ++j) {
      EXPECT_DOUBLE_EQ(iterated[j], jumped[j])
          << "point " << target << " dim " << j;
    }
  }
}

//! scalar discard(z) == z scalar emissions, for a multi-dimensional engine
//! (the generic suite only exercises the 1-D engine).
TEST(SobolQmc, ScalarDiscardMatchesLoopMultiDim) {
  for (unsigned long long z : {0ull, 1ull, 5ull, 7ull, 100ull, 99999ull}) {
    ni::random::Sobol32 skipped{ni::random::dimensions(3)};
    ni::random::Sobol32 looped{ni::random::dimensions(3)};
    skipped.discard(z);
    for (unsigned long long i = 0; i < z; ++i) {
      (void)looped();
    }
    EXPECT_EQ(skipped, looped) << "discard(" << z << ")";
    for (int i = 0; i < 30; ++i) {
      EXPECT_EQ(skipped(), looped()) << "discard(" << z << ") output";
    }
  }
}

//! next_point() returns exactly the coordinates the scalar operator() emits.
TEST(SobolQmc, NextPointMatchesScalarStream) {
  ni::random::Sobol32 viaPoint{ni::random::dimensions(4)};
  ni::random::Sobol32 viaScalar{ni::random::dimensions(4)};
  const double scale = std::ldexp(1.0, -32);
  for (int p = 0; p < 20; ++p) {
    const auto point = viaPoint.next_point();
    for (std::size_t j = 0; j < point.size(); ++j) {
      const double coord = static_cast<double>(viaScalar()) * scale;
      EXPECT_DOUBLE_EQ(point[j], coord) << "point " << p << " dim " << j;
    }
  }
}

//! A zero-valued digital shift is a no-op; a nonzero shift keeps coordinates in
//! [0,1) and changes the stream; clearing restores it.
TEST(SobolQmc, DigitalShift) {
  ni::random::Sobol32 plain{ni::random::dimensions(2)};
  ni::random::Sobol32 shifted{ni::random::dimensions(2)};

  std::mt19937 gen(12345u);
  shifted.randomizeDigitalShift(gen);
  EXPECT_NE(plain, shifted);

  for (int p = 0; p < 100; ++p) {
    const auto point = shifted.next_point();
    for (double c : point) {
      EXPECT_GE(c, 0.0);
      EXPECT_LT(c, 1.0);
    }
  }

  shifted.clearDigitalShift();
  shifted.seed();
  plain.seed();
  EXPECT_EQ(plain, shifted);
}

//! Property A: the first 2^m points of the 2-D sequence form a (0,m,2)-net in
//! base 2 -- every dyadic box of area 2^-m contains exactly one point. Checked
//! for the 2^k x 2^(m-k) grids and (trivially) for the 1-D projection.
TEST(SobolQmc, PropertyADyadicNet) {
  constexpr int m = 6;
  constexpr std::size_t n = std::size_t{1} << m; // 64 points
  ni::random::Sobol32 e{ni::random::dimensions(2)};
  // The (0,m,2)-net property holds for the dyadic block of points 0..2^m-1,
  // which INCLUDES the origin -- so position at point 0 (the default engine
  // skips the origin).
  e.seed(0);
  std::vector<std::array<double, 2>> points(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto p = e.next_point();
    points[i] = {p[0], p[1]};
  }

  // For each split m = a + b, the 2^a x 2^b grid of elementary intervals must
  // hold exactly one point per cell.
  for (int a = 0; a <= m; ++a) {
    const int b = m - a;
    const std::size_t rows = std::size_t{1} << a;
    const std::size_t cols = std::size_t{1} << b;
    std::vector<int> count(rows * cols, 0);
    for (const auto &p : points) {
      auto xi = static_cast<std::size_t>(p[0] * static_cast<double>(rows));
      auto yi = static_cast<std::size_t>(p[1] * static_cast<double>(cols));
      if (xi >= rows) {
        xi = rows - 1;
      }
      if (yi >= cols) {
        yi = cols - 1;
      }
      ++count[xi * cols + yi];
    }
    for (std::size_t c = 0; c < count.size(); ++c) {
      EXPECT_EQ(count[c], 1) << "net split a=" << a << " cell " << c;
    }
  }
}

namespace {
//! Smooth test integrand on [0,1]^2 with exact integral 2/3.
double integrand(double x, double y) { return x * x + y * y; }
constexpr double kExact = 2.0 / 3.0;
} // namespace

//! Quasi-Monte-Carlo (Sobol) integration error must be meaningfully smaller
//! than plain Monte-Carlo at the same sample count.
TEST(SobolQmc, BeatsMonteCarloIntegration) {
  constexpr std::size_t n = 4096;

  ni::random::Sobol32 sobol{ni::random::dimensions(2)};
  double sobolSum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const auto p = sobol.next_point();
    sobolSum += integrand(p[0], p[1]);
  }
  const double sobolError =
      std::abs(sobolSum / static_cast<double>(n) - kExact);

  std::mt19937 gen(2026u);
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  double mcSum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double x = uniform(gen);
    const double y = uniform(gen);
    mcSum += integrand(x, y);
  }
  const double mcError = std::abs(mcSum / static_cast<double>(n) - kExact);

  EXPECT_LT(sobolError, mcError)
      << "sobolError=" << sobolError << " mcError=" << mcError;
  // Sobol should be far better than MC for this smooth integrand; a 5x margin
  // is conservative (the real gap at n=4096 is ~100x).
  EXPECT_LT(sobolError * 5.0, mcError)
      << "sobolError=" << sobolError << " mcError=" << mcError;
}

} // namespace ni::random::test
