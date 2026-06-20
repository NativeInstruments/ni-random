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

//! \file distribution_concept_tests.cpp
//!
//! The distribution analogue of the negative-test block in
//! minstd_rand_tests.cpp. Re-asserts the std oracle (proving the concept is
//! satisfiable / not over-constrained), proves each concept clause bites with
//! stubs that are complete EXCEPT one requirement, and runs a standard-consumer
//! cross-check driving a ni distribution with a ni engine.

#include <istream>
#include <ostream>
#include <random>

#include <gtest/gtest.h>

#include "ni/random/RandomNumberDistribution.hpp"
#include "ni/random/UniformIntDistribution.hpp"
#include "ni/random/Xoshiro256PlusPlus.hpp"

namespace {

using ni::random::RandomNumberDistribution;

//! The std distributions are the concept ORACLE — they prove the concept is
//! satisfiable and not over-constrained. They are NOT a golden-value oracle:
//! std distributions are not specified to be reproducible across
//! implementations, so platform-stable golden values are pinned on the UTL
//! distributions instead (verified by the conformance suite).
static_assert(RandomNumberDistribution<std::uniform_int_distribution<int>>);
static_assert(RandomNumberDistribution<std::uniform_real_distribution<double>>);
static_assert(RandomNumberDistribution<std::normal_distribution<double>>);

//! A type complete in every [rand.req.dist] requirement; the negative stubs
//! below each remove exactly one thing from this baseline.
struct Complete {
  using result_type = double;
  struct param_type {
    using distribution_type = Complete;
    bool operator==(const param_type &) const noexcept = default;
  };
  Complete() = default;
  Complete(const param_type &) {}
  template <class G> result_type operator()(G &) const { return 0.0; }
  template <class G> result_type operator()(G &, const param_type &) const {
    return 0.0;
  }
  void reset() {}
  param_type param() const { return {}; }
  void param(const param_type &) {}
  result_type min() const { return 0.0; }
  result_type max() const { return 0.0; }
  bool operator==(const Complete &) const noexcept = default;
};
template <class C, class Tr>
std::basic_ostream<C, Tr> &operator<<(std::basic_ostream<C, Tr> &os,
                                      const Complete &) {
  return os;
}
template <class C, class Tr>
std::basic_istream<C, Tr> &operator>>(std::basic_istream<C, Tr> &is,
                                      Complete &) {
  return is;
}
static_assert(RandomNumberDistribution<Complete>,
              "the baseline must satisfy the concept");

//! Negative #1: no nested param_type.
struct NoParamType {
  using result_type = double;
  NoParamType() = default;
  template <class G> result_type operator()(G &) const { return 0.0; }
  void reset() {}
  result_type min() const { return 0.0; }
  result_type max() const { return 0.0; }
  bool operator==(const NoParamType &) const noexcept = default;
};
template <class C, class Tr>
std::basic_ostream<C, Tr> &operator<<(std::basic_ostream<C, Tr> &os,
                                      const NoParamType &) {
  return os;
}
template <class C, class Tr>
std::basic_istream<C, Tr> &operator>>(std::basic_istream<C, Tr> &is,
                                      NoParamType &) {
  return is;
}

//! Negative #2: param_type::distribution_type names the wrong type.
struct BadDistributionType {
  using result_type = double;
  struct param_type {
    using distribution_type = int; // wrong: must be BadDistributionType
    bool operator==(const param_type &) const noexcept = default;
  };
  BadDistributionType() = default;
  BadDistributionType(const param_type &) {}
  template <class G> result_type operator()(G &) const { return 0.0; }
  template <class G> result_type operator()(G &, const param_type &) const {
    return 0.0;
  }
  void reset() {}
  param_type param() const { return {}; }
  void param(const param_type &) {}
  result_type min() const { return 0.0; }
  result_type max() const { return 0.0; }
  bool operator==(const BadDistributionType &) const noexcept = default;
};
template <class C, class Tr>
std::basic_ostream<C, Tr> &operator<<(std::basic_ostream<C, Tr> &os,
                                      const BadDistributionType &) {
  return os;
}
template <class C, class Tr>
std::basic_istream<C, Tr> &operator>>(std::basic_istream<C, Tr> &is,
                                      BadDistributionType &) {
  return is;
}

//! Negative #3: missing the operator()(g, p) overload.
struct NoParamOverload {
  using result_type = double;
  struct param_type {
    using distribution_type = NoParamOverload;
    bool operator==(const param_type &) const noexcept = default;
  };
  NoParamOverload() = default;
  NoParamOverload(const param_type &) {}
  template <class G> result_type operator()(G &) const { return 0.0; }
  // no operator()(G&, const param_type&)
  void reset() {}
  param_type param() const { return {}; }
  void param(const param_type &) {}
  result_type min() const { return 0.0; }
  result_type max() const { return 0.0; }
  bool operator==(const NoParamOverload &) const noexcept = default;
};
template <class C, class Tr>
std::basic_ostream<C, Tr> &operator<<(std::basic_ostream<C, Tr> &os,
                                      const NoParamOverload &) {
  return os;
}
template <class C, class Tr>
std::basic_istream<C, Tr> &operator>>(std::basic_istream<C, Tr> &is,
                                      NoParamOverload &) {
  return is;
}

//! Negative #4: complete EXCEPT no stream operators (none defined for it).
struct NoStream {
  using result_type = double;
  struct param_type {
    using distribution_type = NoStream;
    bool operator==(const param_type &) const noexcept = default;
  };
  NoStream() = default;
  NoStream(const param_type &) {}
  template <class G> result_type operator()(G &) const { return 0.0; }
  template <class G> result_type operator()(G &, const param_type &) const {
    return 0.0;
  }
  void reset() {}
  param_type param() const { return {}; }
  void param(const param_type &) {}
  result_type min() const { return 0.0; }
  result_type max() const { return 0.0; }
  bool operator==(const NoStream &) const noexcept = default;
};

static_assert(!RandomNumberDistribution<NoParamType>,
              "a type without a nested param_type must be rejected");
static_assert(!RandomNumberDistribution<BadDistributionType>,
              "param_type::distribution_type must name the distribution");
static_assert(!RandomNumberDistribution<NoParamOverload>,
              "a type without operator()(g, p) must be rejected");
static_assert(!RandomNumberDistribution<NoStream>,
              "a type without stream operators must be rejected");

TEST(RandomNumberDistributionConcept, AcceptsOracleRejectsBrokenStubs) {
  EXPECT_TRUE(RandomNumberDistribution<std::uniform_int_distribution<int>>);
  EXPECT_TRUE(
      RandomNumberDistribution<ni::random::UniformIntDistribution<int>>);
  EXPECT_FALSE(RandomNumberDistribution<NoParamType>);
  EXPECT_FALSE(RandomNumberDistribution<BadDistributionType>);
  EXPECT_FALSE(RandomNumberDistribution<NoParamOverload>);
  EXPECT_FALSE(RandomNumberDistribution<NoStream>);
}

//! Standard-consumer cross-check: drive a ni distribution with a ni engine and
//! confirm the draws land in range — the drop-in integration check, parallel to
//! MinstdRandStandardConsumers.
TEST(DistributionStandardConsumers, UniformIntInRange) {
  ni::random::Xoshiro256PlusPlus engine(12345u);
  ni::random::UniformIntDistribution<int> dist(0, 99); // inclusive [0, 99]
  for (int i = 0; i < 1000; ++i) {
    const int v = dist(engine);
    EXPECT_GE(v, 0);
    EXPECT_LE(v, 99);
  }
}

} // namespace
