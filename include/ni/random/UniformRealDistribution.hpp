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

//! \file UniformRealDistribution.hpp
//!
//! Uniform real distribution via an optimized generate_canonical, the
//! std::uniform_real_distribution analogue. Imported from DmitriBogdanov/UTL
//! (utl::random, random.hpp); the sampling body is unmodified. Made conformant
//! to ni::random::RandomNumberDistribution by normalizing params()->param(),
//! adding param_type::distribution_type + param_type equality, const-qualifying
//! operator==, and adding CharT-generic stream operators (max-precision so the
//! floating-point params round-trip exactly). See
//! plans/import-utl-distributions.md and provenance random-repos/UTL.

#include "ni/random/RandomNumberDistribution.hpp"
#include "ni/random/detail/UtlDistributionMath.hpp"

#include <cassert>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>

namespace ni::random {

template <class T = double, detail::require_float<T> = true>
struct UniformRealDistribution {
  using result_type = T;

  struct param_type {
    result_type min = 0;
    result_type max = std::numeric_limits<result_type>::max();

    using distribution_type = UniformRealDistribution;

    bool operator==(const param_type &) const noexcept = default;
  } pars{};

  constexpr UniformRealDistribution() = default;
  constexpr UniformRealDistribution(T min, T max) noexcept : pars({min, max}) {
    assert(min < max);
  }
  constexpr UniformRealDistribution(const param_type &p) noexcept : pars(p) {
    assert(p.min < p.max);
  }

  template <class Gen>
  constexpr result_type operator()(Gen &gen) const noexcept(noexcept(gen())) {
    return this->pars.min + detail::generate_canonical<result_type>(gen) *
                                (this->pars.max - this->pars.min);
  }

  template <class Gen>
  constexpr T operator()(Gen &gen, const param_type &p) const
      noexcept(noexcept(gen())) {
    assert(p.min < p.max);
    return p.min +
           detail::generate_canonical<result_type>(gen) * (p.max - p.min);
  } // for std-compatibility

  constexpr void reset() const noexcept {
  } // nothing to reset, provided for std-API compatibility
  [[nodiscard]] constexpr param_type param() const noexcept {
    return this->pars;
  }
  constexpr void param(const param_type &p) noexcept {
    *this = UniformRealDistribution(p);
  }
  constexpr result_type a() const noexcept { return this->pars.min; }
  constexpr result_type b() const noexcept { return this->pars.max; }
  constexpr result_type min() const noexcept { return this->pars.min; }
  constexpr result_type max() const noexcept { return this->pars.max; }

  constexpr bool
  operator==(const UniformRealDistribution &other) const noexcept {
    return this->a() == other.a() && this->b() == other.b();
  }
  constexpr bool
  operator!=(const UniformRealDistribution &other) const noexcept {
    return !(*this == other);
  }
};

//! Stream insertion/extraction satisfying the DistributionStreamSerializable
//! clause. The two param fields are written at max_digits10 precision so they
//! round-trip exactly; format flags, fill, and precision are restored.
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os,
           const UniformRealDistribution<T> &dist) {
  const auto flags = os.flags();
  const auto fill = os.fill();
  const auto precision = os.precision();
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(os.widen(' '));
  os.precision(std::numeric_limits<T>::max_digits10);
  os << dist.min() << os.widen(' ') << dist.max();
  os.flags(flags);
  os.fill(fill);
  os.precision(precision);
  return os;
}

template <class CharT, class Traits, class T>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is,
           UniformRealDistribution<T> &dist) {
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  T lo{};
  T hi{};
  is >> lo >> hi;
  is.flags(flags);
  if (is) {
    dist.param(typename UniformRealDistribution<T>::param_type{lo, hi});
  }
  return is;
}

static_assert(RandomNumberDistribution<UniformRealDistribution<double>>);

} // namespace ni::random
