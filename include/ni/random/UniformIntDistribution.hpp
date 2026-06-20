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

//! \file UniformIntDistribution.hpp
//!
//! Bias-free uniform integer distribution (Lemire), the
//! std::uniform_int_distribution analogue. Imported from DmitriBogdanov/UTL
//! (utl::random, random.hpp); the sampling body is unmodified. Made conformant
//! to ni::random::RandomNumberDistribution by normalizing params()->param(),
//! adding param_type::distribution_type + param_type equality, const-qualifying
//! operator==, and adding CharT-generic stream operators. See
//! plans/import-utl-distributions.md and provenance random-repos/UTL.

#include "ni/random/RandomNumberDistribution.hpp"
#include "ni/random/detail/UtlDistributionMath.hpp"

#include <cassert>
#include <cstddef>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>

namespace ni::random {

template <class T = int, detail::require_integral<T> = true>
struct UniformIntDistribution {
  using result_type = T;

  struct param_type {
    result_type min = 0;
    result_type max = std::numeric_limits<result_type>::max();

    using distribution_type = UniformIntDistribution;

    bool operator==(const param_type &) const noexcept = default;
  };

  constexpr UniformIntDistribution() = default;
  constexpr UniformIntDistribution(T min, T max) noexcept : pars({min, max}) {
    assert(min < max);
  }
  constexpr UniformIntDistribution(const param_type &p) noexcept : pars(p) {
    assert(p.min < p.max);
  }

  template <class Gen>
  constexpr T operator()(Gen &gen) const noexcept(noexcept(gen())) {
    return detail::generate_uniform_int<result_type>(gen, this->pars.min,
                                                     this->pars.max);
  }

  template <class Gen>
  constexpr T operator()(Gen &gen, const param_type &p) const
      noexcept(noexcept(gen())) {
    assert(p.min < p.max);
    return detail::generate_uniform_int<result_type>(gen, p.min, p.max);
  } // for std-compatibility

  constexpr void reset() const noexcept {
  } // nothing to reset, provided for std-compatibility
  [[nodiscard]] constexpr param_type param() const noexcept {
    return this->pars;
  }
  constexpr void param(const param_type &p) noexcept {
    *this = UniformIntDistribution(p);
  }
  [[nodiscard]] constexpr result_type a() const noexcept {
    return this->pars.min;
  }
  [[nodiscard]] constexpr result_type b() const noexcept {
    return this->pars.max;
  }
  [[nodiscard]] constexpr result_type min() const noexcept {
    return this->pars.min;
  }
  [[nodiscard]] constexpr result_type max() const noexcept {
    return this->pars.max;
  }

  constexpr bool
  operator==(const UniformIntDistribution &other) const noexcept {
    return this->a() == other.a() && this->b() == other.b();
  }
  constexpr bool
  operator!=(const UniformIntDistribution &other) const noexcept {
    return !(*this == other);
  }

private:
  param_type pars{};
};

//! Stream insertion/extraction satisfying the DistributionStreamSerializable
//! clause. The two param fields are written as space-separated decimal
//! integers; format flags and fill are forced to a known state and restored so
//! insertion does not perturb the stream.
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os,
           const UniformIntDistribution<T> &dist) {
  const auto flags = os.flags();
  const auto fill = os.fill();
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(os.widen(' '));
  os << +dist.min() << os.widen(' ') << +dist.max();
  os.flags(flags);
  os.fill(fill);
  return os;
}

template <class CharT, class Traits, class T>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is,
           UniformIntDistribution<T> &dist) {
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  // Read through a wider signed type so 8-bit result_types are parsed as
  // integers rather than characters; the param_type fields are then narrowed.
  long long lo = 0;
  long long hi = 0;
  is >> lo >> hi;
  is.flags(flags);
  if (is) {
    dist.param(typename UniformIntDistribution<T>::param_type{
        static_cast<T>(lo), static_cast<T>(hi)});
  }
  return is;
}

static_assert(RandomNumberDistribution<UniformIntDistribution<int>>);

} // namespace ni::random
