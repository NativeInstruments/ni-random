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

//! \file SimpleUniformRealDistribution.hpp
//!
//! ni::random::SimpleUniformRealDistribution — a minimal uniform real
//! distribution that maps the high bits of a single generator draw onto a
//! canonical [0, 1) value, then affine-scales it into [a, b).
//!
//! The sampling body is the original "simple" mapping (one generator draw,
//! high bits shifted into the mantissa). It has been made conformant to
//! ni::random::RandomNumberDistribution by giving it the [rand.req.dist]
//! surface the std distributions provide: a nested param_type (carrying its
//! distribution_type and equality), a default constructor, param()/reset()/
//! min()/max() accessors, const-qualified equality, and CharT-generic stream
//! operators (max-precision so the floating-point params round-trip exactly).
//! The accessors and stream operators mirror UniformRealDistribution.

#include "ni/random/RandomNumberDistribution.hpp"

#include <cassert>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>

namespace ni::random {

template <class T = double> class SimpleUniformRealDistribution {
public:
  static_assert(std::is_floating_point_v<T>);

  using result_type = T;

  struct param_type {
    result_type a = 0;
    result_type b = 1;

    using distribution_type = SimpleUniformRealDistribution;

    bool operator==(const param_type &) const noexcept = default;
  };

  SimpleUniformRealDistribution() = default;
  SimpleUniformRealDistribution(T a, T b) : m_pars{a, b} { assert(a < b); }
  explicit SimpleUniformRealDistribution(const param_type &p) : m_pars{p} {
    assert(p.a < p.b);
  }

  template <class Gen> result_type operator()(Gen &gen) const {
    return sample(gen, m_pars);
  }

  template <class Gen>
  result_type operator()(Gen &gen, const param_type &p) const {
    assert(p.a < p.b);
    return sample(gen, p);
  }

  void reset() const noexcept {}
  [[nodiscard]] param_type param() const noexcept { return m_pars; }
  void param(const param_type &p) noexcept {
    *this = SimpleUniformRealDistribution(p);
  }
  [[nodiscard]] result_type a() const noexcept { return m_pars.a; }
  [[nodiscard]] result_type b() const noexcept { return m_pars.b; }
  [[nodiscard]] result_type min() const noexcept { return m_pars.a; }
  [[nodiscard]] result_type max() const noexcept { return m_pars.b; }

  bool operator==(const SimpleUniformRealDistribution &other) const noexcept {
    return m_pars == other.m_pars;
  }
  bool operator!=(const SimpleUniformRealDistribution &other) const noexcept {
    return !(*this == other);
  }

private:
  template <class Gen>
  static result_type sample(Gen &gen, const param_type &p) {
    static_assert(sizeof(typename Gen::result_type) == sizeof(float) ||
                  sizeof(typename Gen::result_type) == sizeof(double));
    const result_type scale = p.b - p.a;
    const result_type shift = p.a;
    if constexpr (std::is_same_v<T, double>) {
      return (gen() >> 11u) * 0x1.0p-53 * scale + shift;
    } else {
      if constexpr (sizeof(typename Gen::result_type) == sizeof(double)) {
        return ((gen() >> 32u) >> 8) * 0x1.0p-24f * scale + shift;
      } else {
        return (gen() >> 8) * 0x1.0p-24f * scale + shift;
      }
    }
  }

  param_type m_pars{};
};

//! Stream insertion/extraction satisfying the DistributionStreamSerializable
//! clause. The two param fields are written at max_digits10 precision so they
//! round-trip exactly; format flags, fill, and precision are restored.
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os,
           const SimpleUniformRealDistribution<T> &dist) {
  const auto flags = os.flags();
  const auto fill = os.fill();
  const auto precision = os.precision();
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(os.widen(' '));
  os.precision(std::numeric_limits<T>::max_digits10);
  os << dist.a() << os.widen(' ') << dist.b();
  os.flags(flags);
  os.fill(fill);
  os.precision(precision);
  return os;
}

template <class CharT, class Traits, class T>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is,
           SimpleUniformRealDistribution<T> &dist) {
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  T lo{};
  T hi{};
  is >> lo >> hi;
  is.flags(flags);
  if (is) {
    dist.param(typename SimpleUniformRealDistribution<T>::param_type{lo, hi});
  }
  return is;
}

static_assert(RandomNumberDistribution<SimpleUniformRealDistribution<double>>);

} // namespace ni::random
