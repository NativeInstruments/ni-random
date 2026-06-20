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

//! \file SobolDirectionNumbers.hpp
//!
//! Direction-number machinery for the Sobol low-discrepancy generator: the
//! embedded default table (Joe-Kuo construction), a loader for the Joe-Kuo
//! "new-joe-kuo-6.x" file format, and the builder that turns a polynomial spec
//! into the per-dimension W-bit direction integers V_{k,j}.
//!
//! The embedded numbers are taken verbatim from the canonical direction-number
//! file new-joe-kuo-6.21201 (Stephen Joe & Frances Kuo, search criterion D(6)),
//! https://web.maths.unsw.edu.au/~fkuo/sobol/. Columns are
//! `d s a m_1 ... m_s`: d = dimension, s = degree of the primitive polynomial,
//! a = packed interior coefficients of that polynomial, m_k = initial direction
//! integers (each odd, m_k < 2^k). Only dimensions 2..51 are embedded
//! (dimension 1 is the radical inverse and needs no polynomial); use
//! loadJoeKuoDirectionNumbers() with the full file to go higher. The full
//! multi-MB file is deliberately NOT bundled.
//!
//! Original ni-random code implemented from the published Antonov-Saleev /
//! Joe-Kuo construction; no third-party source is copied or included, hence no
//! license notice. See plans/qrng-sobol.md.

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ni::random::detail {

//! One primitive polynomial + its initial direction integers, for a single
//! Sobol dimension j >= 2 (1-based). `coefficients` packs the interior
//! coefficients a_1..a_{degree-1} of the degree-`degree` primitive polynomial:
//! a_i = (coefficients >> (degree-1-i)) & 1, so a_1 is the most significant
//! interior bit and a_{degree-1} the least. `initialM` holds m_1..m_degree.
struct SobolDirectionSpec {
  unsigned degree{};
  std::uint32_t coefficients{};
  std::vector<std::uint32_t> initialM;
};

//! The embedded default direction-number table for dimensions 2..51. Index 0
//! corresponds to dimension d = 2 (the first dimension that needs a
//! polynomial); dimension 1, the radical inverse, is handled separately by the
//! builder. Built once and cached. Values are the first 50 data lines of
//! new-joe-kuo-6.21201.
inline const std::vector<SobolDirectionSpec> &defaultSobolDirectionTable() {
  static const std::vector<SobolDirectionSpec> table = {
      // d   s  a   m_1..m_s
      {1, 0, {1}},                             // d=2
      {2, 1, {1, 3}},                          // d=3
      {3, 1, {1, 3, 1}},                       // d=4
      {3, 2, {1, 1, 1}},                       // d=5
      {4, 1, {1, 1, 3, 3}},                    // d=6
      {4, 4, {1, 3, 5, 13}},                   // d=7
      {5, 2, {1, 1, 5, 5, 17}},                // d=8
      {5, 4, {1, 1, 5, 5, 5}},                 // d=9
      {5, 7, {1, 1, 7, 11, 19}},               // d=10
      {5, 11, {1, 1, 5, 1, 1}},                // d=11
      {5, 13, {1, 1, 1, 3, 11}},               // d=12
      {5, 14, {1, 3, 5, 5, 31}},               // d=13
      {6, 1, {1, 3, 3, 9, 7, 49}},             // d=14
      {6, 13, {1, 1, 1, 15, 21, 21}},          // d=15
      {6, 16, {1, 3, 1, 13, 27, 49}},          // d=16
      {6, 19, {1, 1, 1, 15, 7, 5}},            // d=17
      {6, 22, {1, 3, 1, 15, 13, 25}},          // d=18
      {6, 25, {1, 1, 5, 5, 19, 61}},           // d=19
      {7, 1, {1, 3, 7, 11, 23, 15, 103}},      // d=20
      {7, 4, {1, 3, 7, 13, 13, 15, 69}},       // d=21
      {7, 7, {1, 1, 3, 13, 7, 35, 63}},        // d=22
      {7, 8, {1, 3, 5, 9, 1, 25, 53}},         // d=23
      {7, 14, {1, 3, 1, 13, 9, 35, 107}},      // d=24
      {7, 19, {1, 3, 1, 5, 27, 61, 31}},       // d=25
      {7, 21, {1, 1, 5, 11, 19, 41, 61}},      // d=26
      {7, 28, {1, 3, 5, 3, 3, 13, 69}},        // d=27
      {7, 31, {1, 1, 7, 13, 1, 19, 1}},        // d=28
      {7, 32, {1, 3, 7, 5, 13, 19, 59}},       // d=29
      {7, 37, {1, 1, 3, 9, 25, 29, 41}},       // d=30
      {7, 41, {1, 3, 5, 13, 23, 1, 55}},       // d=31
      {7, 42, {1, 3, 7, 3, 13, 59, 17}},       // d=32
      {7, 50, {1, 3, 1, 3, 5, 53, 69}},        // d=33
      {7, 55, {1, 1, 5, 5, 23, 33, 13}},       // d=34
      {7, 56, {1, 1, 7, 7, 1, 61, 123}},       // d=35
      {7, 59, {1, 1, 7, 9, 13, 61, 49}},       // d=36
      {7, 62, {1, 3, 3, 5, 3, 55, 33}},        // d=37
      {8, 14, {1, 3, 1, 15, 31, 13, 49, 245}}, // d=38
      {8, 21, {1, 3, 5, 15, 31, 59, 63, 97}},  // d=39
      {8, 22, {1, 3, 1, 11, 11, 11, 77, 249}}, // d=40
      {8, 38, {1, 3, 1, 11, 27, 43, 71, 9}},   // d=41
      {8, 47, {1, 1, 7, 15, 21, 11, 81, 45}},  // d=42
      {8, 49, {1, 3, 7, 3, 25, 31, 65, 79}},   // d=43
      {8, 50, {1, 3, 1, 1, 19, 11, 3, 205}},   // d=44
      {8, 52, {1, 1, 5, 9, 19, 21, 29, 157}},  // d=45
      {8, 56, {1, 3, 7, 11, 1, 33, 89, 185}},  // d=46
      {8, 67, {1, 3, 3, 3, 15, 9, 79, 71}},    // d=47
      {8, 70, {1, 3, 7, 11, 15, 39, 119, 27}}, // d=48
      {8, 84, {1, 1, 3, 1, 11, 31, 97, 225}},  // d=49
      {8, 97, {1, 1, 1, 3, 23, 43, 57, 177}},  // d=50
      {8, 103, {1, 3, 7, 7, 17, 17, 37, 71}},  // d=51
  };
  return table;
}

//! Largest dimension the embedded table supports out of the box (the radical-
//! inverse dimension 1 plus one per embedded polynomial).
inline std::size_t maxDefaultSobolDimension() {
  return 1 + defaultSobolDirectionTable().size();
}

//! Parse direction numbers from a Joe-Kuo "new-joe-kuo-6.x" stream. Each data
//! line is `d s a m_1 ... m_s`; an optional non-numeric header line is skipped.
//! Returns specs for dimensions 2..D in file order (the radical-inverse
//! dimension 1 has no line). Throws std::runtime_error on a malformed line.
inline std::vector<SobolDirectionSpec>
loadJoeKuoDirectionNumbers(std::istream &in) {
  std::vector<SobolDirectionSpec> table;
  std::string line;
  while (std::getline(in, line)) {
    // Skip blank lines and the column header (any line whose first
    // non-space character is not a digit).
    std::size_t pos = line.find_first_not_of(" \t\r\n");
    if (pos == std::string::npos) {
      continue;
    }
    if (line[pos] < '0' || line[pos] > '9') {
      continue;
    }
    std::istringstream ls(line);
    unsigned long long d = 0;
    unsigned long long s = 0;
    unsigned long long a = 0;
    if (!(ls >> d >> s >> a)) {
      throw std::runtime_error("Sobol direction file: malformed line");
    }
    SobolDirectionSpec spec;
    spec.degree = static_cast<unsigned>(s);
    spec.coefficients = static_cast<std::uint32_t>(a);
    spec.initialM.resize(spec.degree);
    for (unsigned k = 0; k < spec.degree; ++k) {
      unsigned long long m = 0;
      if (!(ls >> m)) {
        throw std::runtime_error("Sobol direction file: too few m values");
      }
      spec.initialM[k] = static_cast<std::uint32_t>(m);
    }
    table.push_back(std::move(spec));
  }
  return table;
}

//! Build the per-dimension direction integers for `dimension` dimensions and
//! word width W. result[j] is V_{1..W, j} as W-bit words, with result[j][k-1] =
//! V_{k,j}. Dimension 0 (1-based dimension 1) is the radical inverse;
//! dimensions j >= 1 use table[j-1] (so the table must cover at least
//! `dimension - 1` polynomials). Throws std::out_of_range if the table is too
//! small.
template <class UIntType, unsigned W>
std::vector<std::array<UIntType, W>>
buildSobolDirectionNumbers(std::size_t dimension,
                           const std::vector<SobolDirectionSpec> &table) {
  static_assert(W >= 1 && W <= sizeof(UIntType) * 8,
                "W must be in [1, bit width of UIntType]");
  if (dimension >= 1 && dimension - 1 > table.size()) {
    throw std::out_of_range(
        "Sobol: requested dimension exceeds available direction numbers");
  }
  std::vector<std::array<UIntType, W>> v(dimension);

  // Dimension 1 (j = 0): radical inverse base 2, V_k = 2^(W-k).
  if (dimension >= 1) {
    for (unsigned k = 1; k <= W; ++k) {
      v[0][k - 1] = static_cast<UIntType>(UIntType{1} << (W - k));
    }
  }

  for (std::size_t j = 1; j < dimension; ++j) {
    const SobolDirectionSpec &spec = table[j - 1];
    const unsigned s = spec.degree;
    std::array<UIntType, W> &vj = v[j];

    // Seed V_k = m_k << (W-k) for k = 1..s (the initial direction integers).
    for (unsigned k = 1; k <= s && k <= W; ++k) {
      vj[k - 1] = static_cast<UIntType>(
          static_cast<UIntType>(spec.initialM[k - 1]) << (W - k));
    }
    // Recurrence for k = s+1..W (direction-integer / V-space form of the
    // Joe-Kuo m_k recurrence; see file header):
    //   V_k = V_{k-s} ^ (V_{k-s} >> s) ^ sum_{i=1}^{s-1} a_i V_{k-i}
    for (unsigned k = s + 1; k <= W; ++k) {
      UIntType value = vj[k - s - 1];
      value ^= static_cast<UIntType>(vj[k - s - 1] >> s);
      for (unsigned i = 1; i < s; ++i) {
        const unsigned ai = (spec.coefficients >> (s - 1 - i)) & 1u;
        if (ai != 0u) {
          value ^= vj[k - i - 1];
        }
      }
      vj[k - 1] = value;
    }
  }
  return v;
}

} // namespace ni::random::detail
