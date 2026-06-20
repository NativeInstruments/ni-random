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

//! \file SimpleUniformIntDistribution.hpp
//!
//! ni::random::SimpleUniformIntDistribution — a minimal uniform integer
//! distribution built on SimpleUniformRealDistribution: it draws a real value
//! uniformly over [a - 0.5, b + 0.5) and rounds to the nearest integer, giving
//! a uniform inclusive [a, b].
//!
//! The sampling body is the original "simple" round-of-real mapping. It has
//! been made conformant to ni::random::RandomNumberDistribution by giving it
//! the [rand.req.dist] surface the std distributions provide: a nested
//! param_type (carrying its distribution_type and equality), a default
//! constructor, param()/reset()/min()/max() accessors, const-qualified
//! equality, and CharT-generic stream operators. The accessors and stream
//! operators mirror UniformIntDistribution.

#include "ni/random/RandomNumberDistribution.hpp"
#include "ni/random/SimpleUniformRealDistribution.hpp"

#include <cassert>
#include <cmath>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>

namespace ni::random {

template <class T = unsigned> class SimpleUniformIntDistribution {
public:
  static_assert(std::is_unsigned_v<T>);

  using result_type = T;

  struct param_type {
    result_type a = 0;
    result_type b = std::numeric_limits<result_type>::max();

    using distribution_type = SimpleUniformIntDistribution;

    bool operator==(const param_type &) const noexcept = default;
  };

  SimpleUniformIntDistribution() = default;
  SimpleUniformIntDistribution(T a, T b) : m_pars{a, b} { assert(a < b); }
  explicit SimpleUniformIntDistribution(const param_type &p) : m_pars{p} {
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
    *this = SimpleUniformIntDistribution(p);
  }
  [[nodiscard]] result_type a() const noexcept { return m_pars.a; }
  [[nodiscard]] result_type b() const noexcept { return m_pars.b; }
  [[nodiscard]] result_type min() const noexcept { return m_pars.a; }
  [[nodiscard]] result_type max() const noexcept { return m_pars.b; }

  bool operator==(const SimpleUniformIntDistribution &other) const noexcept {
    return m_pars == other.m_pars;
  }
  bool operator!=(const SimpleUniformIntDistribution &other) const noexcept {
    return !(*this == other);
  }

private:
  template <class Gen>
  static result_type sample(Gen &gen, const param_type &p) {
    static_assert(sizeof(typename Gen::result_type) >= sizeof(T));
    SimpleUniformRealDistribution<float> realDist(
        static_cast<float>(p.a) - 0.5f, static_cast<float>(p.b) + 0.5f);
    return static_cast<result_type>(std::llround(realDist(gen)));
  }

  param_type m_pars{};
};

//! Stream insertion/extraction satisfying the DistributionStreamSerializable
//! clause. The two param fields are written as space-separated decimal
//! integers; format flags and fill are forced to a known state and restored so
//! insertion does not perturb the stream.
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os,
           const SimpleUniformIntDistribution<T> &dist) {
  const auto flags = os.flags();
  const auto fill = os.fill();
  os.flags(std::ios_base::dec | std::ios_base::left);
  os.fill(os.widen(' '));
  os << +dist.a() << os.widen(' ') << +dist.b();
  os.flags(flags);
  os.fill(fill);
  return os;
}

template <class CharT, class Traits, class T>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is,
           SimpleUniformIntDistribution<T> &dist) {
  const auto flags = is.flags();
  is.flags(std::ios_base::dec | std::ios_base::skipws);
  // Read through a wider unsigned type so 8-bit result_types are parsed as
  // integers rather than characters; the param_type fields are then narrowed.
  unsigned long long lo = 0;
  unsigned long long hi = 0;
  is >> lo >> hi;
  is.flags(flags);
  if (is) {
    dist.param(typename SimpleUniformIntDistribution<T>::param_type{
        static_cast<T>(lo), static_cast<T>(hi)});
  }
  return is;
}

static_assert(RandomNumberDistribution<SimpleUniformIntDistribution<unsigned>>);

} // namespace ni::random
