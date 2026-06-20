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
//
// Portions of this file derive from third-party software; the original
// copyright and license notice is retained below and continues to apply.
//
// clang-format on

// clang-format off
//----------------------------------------------------------------------------------------
//
//	DmitriBogdanov/UTL — utl::random
//	https://github.com/DmitriBogdanov/UTL
//
//	MIT License
//
//	Copyright (c) 2023 Dmitri Bogdanov
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//----------------------------------------------------------------------------------------
// clang-format on

#pragma once

//! \file NormalDistribution.hpp
//!
//! Marsaglia-polar normal distribution, the std::normal_distribution analogue.
//! Imported from DmitriBogdanov/UTL (utl::random, random.hpp); the sampling
//! body is unmodified. STATEFUL: the polar pair's second value is cached in
//! saved/saved_available, so reset() clears the cache and the stream operators
//! serialize all four fields (mean, stddev, saved, saved_available) — required
//! for the round-trip postcondition since operator== compares the cache. Made
//! conformant to ni::random::RandomNumberDistribution by adding
//! param_type::distribution_type + param_type equality, const-qualifying
//! operator==, and adding CharT-generic stream operators. See
//! plans/import-utl-distributions.md and provenance random-repos/UTL.

#include "ni/random/RandomNumberDistribution.hpp"
#include "ni/random/detail/UtlDistributionMath.hpp"

#include <cassert>
#include <cmath>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>

namespace ni::random {

template <class T = double, detail::require_float<T> = true>
struct NormalDistribution {
  using result_type = T;

  struct param_type {
    result_type mean = 0;
    result_type stddev = 1;

    using distribution_type = NormalDistribution;

    bool operator==(const param_type &) const noexcept = default;
  } pars{};

private:
  // Marsaglia Polar algorithm generates values in pairs so we need to cache the
  // 2nd one
  result_type saved = 0;
  bool saved_available = false;

  // Implementation of Marsaglia Polar method for N(0, 1) based on libstdc++,
  // the algorithm is exactly the same, except we use a faster uniform
  // distribution ('generate_canonical()' that was implemented earlier)
  //
  // Note 1:
  // Even if 'generate_canonical()' produced [0, 1] range instead of [0, 1),
  // this would not be an issue since Marsaglia Polar is a rejection method and
  // does not care about the inclusion of upper-boundaries, they get rejected by
  // 'r2 > T(1)' check
  //
  // Note 2:
  // As far as normal distributions go we have 3 options: Box-Muller, Marsaglia
  // Polar, Ziggurat. Box-Muller performance is similar to Marsaglia Polar, but
  // it has issues working with [0, 1] 'generate_canonical()'. Ziggurat is
  // usually ~50% faster, but involves several KB of lookup tables and a MUCH
  // more cumbersome and difficult to generalize implementation. For the sake of
  // robustness we will stick to Polar method for now.
  //
  // Note 3:
  // Not 'constexpr' due to the <cmath> nonsense, can't do anything about it,
  // will be fixed with C++23.
  //
  template <class Gen> result_type generate_standard_normal(Gen &gen) noexcept {
    if (this->saved_available) {
      this->saved_available = false;
      return this->saved;
    }

    result_type x, y, r2;

    do {
      x = T(2) * detail::generate_canonical<result_type>(gen) - T(1);
      y = T(2) * detail::generate_canonical<result_type>(gen) - T(1);
      r2 = x * x + y * y;
    } while (r2 > T(1) || r2 == T(0));

    const result_type mult = std::sqrt(-2 * std::log(r2) / r2);

    this->saved_available = true;
    this->saved = x * mult;

    return y * mult;
  }

public:
  constexpr NormalDistribution() = default;
  constexpr NormalDistribution(T mean, T stddev) noexcept
      : pars({mean, stddev}) {
    assert(stddev >= T(0));
  }
  constexpr NormalDistribution(const param_type &p) noexcept : pars(p) {
    assert(p.stddev >= T(0));
  }

  template <class Gen> result_type operator()(Gen &gen) noexcept {
    return this->generate_standard_normal(gen) * this->pars.stddev +
           this->pars.mean;
  }

  template <class Gen>
  result_type operator()(Gen &gen, const param_type &params) noexcept {
    assert(params.stddev >= T(0));
    return this->generate_standard_normal(gen) * params.stddev + params.mean;
  }

  constexpr void reset() noexcept {
    this->saved = 0;
    this->saved_available = false;
  }
  [[nodiscard]] constexpr param_type param() const noexcept {
    return this->pars;
  }
  constexpr void param(const param_type &p) noexcept {
    *this = NormalDistribution(p);
  }
  [[nodiscard]] constexpr result_type mean() const noexcept {
    return this->pars.mean;
  }
  [[nodiscard]] constexpr result_type stddev() const noexcept {
    return this->pars.stddev;
  }
  [[nodiscard]] constexpr result_type min() const noexcept {
    return std::numeric_limits<result_type>::lowest();
  }
  [[nodiscard]] constexpr result_type max() const noexcept {
    return std::numeric_limits<result_type>::max();
  }

  constexpr bool operator==(const NormalDistribution &other) const noexcept {
    return this->mean() == other.mean() && this->stddev() == other.stddev() &&
           this->saved_available == other.saved_available &&
           this->saved == other.saved;
  }
  constexpr bool operator!=(const NormalDistribution &other) const noexcept {
    return !(*this == other);
  }

  // The stream operators serialize the full state (mean, stddev, saved,
  // saved_available); they need access to the private cache fields.
  template <class CharT, class Traits, class U>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &os,
             const NormalDistribution<U> &dist);

  template <class CharT, class Traits, class U>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &is,
             NormalDistribution<U> &dist);
};

//! Stream insertion satisfying DistributionStreamSerializable. Writes the full
//! state — mean, stddev, saved, saved_available — at max_digits10 precision so
//! a deserialized distribution compares equal (operator== inspects the cache)
//! and continues the identical sequence. Format flags, fill, and precision are
//! restored.
template <class CharT, class Traits, class U>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os,
           const NormalDistribution<U> &dist) {
  const auto flags = os.flags();
  const auto fill = os.fill();
  const auto precision = os.precision();
  const CharT space = os.widen(' ');
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(space);
  os.precision(std::numeric_limits<U>::max_digits10);
  os << dist.pars.mean << space << dist.pars.stddev << space << dist.saved
     << space << static_cast<int>(dist.saved_available);
  os.flags(flags);
  os.fill(fill);
  os.precision(precision);
  return os;
}

template <class CharT, class Traits, class U>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is, NormalDistribution<U> &dist) {
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  U mean{};
  U stddev{};
  U saved{};
  int savedAvailable = 0;
  is >> mean >> stddev >> saved >> savedAvailable;
  is.flags(flags);
  if (is) {
    dist.pars.mean = mean;
    dist.pars.stddev = stddev;
    dist.saved = saved;
    dist.saved_available = (savedAvailable != 0);
  }
  return is;
}

static_assert(RandomNumberDistribution<NormalDistribution<double>>);

} // namespace ni::random
