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

//! \file SobolEngine.hpp
//!
//! Sobol low-discrepancy (quasi-random) sequence generator, shared by the
//! predefined ni::random aliases Sobol32 / Sobol64 (see Sobol32.hpp /
//! Sobol64.hpp). Parameterized on the result_type and the word width W.
//!
//! Sobol is a base-2 digital (t,s)-sequence: its low-discrepancy property is a
//! property of the whole d-dimensional POINT, not of a scalar stream. The
//! C++ [rand.req.eng] surface is scalar, so -- following Boost.Random's sobol,
//! scipy.stats.qmc, and the Joe-Kuo reference -- the DIMENSION is fixed at
//! construction and the scalar operator() walks the coordinates of the current
//! point in order, advancing to the next point after `dimension` calls.
//!
//! ==========================================================================
//! FOOTGUN -- READ THIS: operator() exists only for engine-concept
//! compatibility. Callers MUST consume exactly `dimension()` values per point.
//! Feeding this engine to a multi-call <random> distribution
//! (std::uniform_real_distribution, std::generate_canonical, ...) consumes an
//! unpredictable number of draws per call and SILENTLY DESTROYS the
//! low-discrepancy property. For correct quasi-Monte-Carlo use, call
//! next_point() and convert whole points yourself.
//! ==========================================================================
//!
//! Emission order is the Antonov-Saleev / Gray-code order (the same order
//! scipy / Boost / Joe-Kuo emit), which is the canonical Sobol enumeration --
//! the SAME point set as the natural radical-inverse (van der Corput) sequence
//! but in Gray-code order. See plans/qrng-sobol.md.
//!
//! Original ni-random code implemented from the published Antonov-Saleev /
//! Joe-Kuo construction; no third-party source is copied or included, hence no
//! license notice.

#include "ni/random/SeedSequence.hpp"
#include "ni/random/detail/SobolDirectionNumbers.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <span>
#include <type_traits>
#include <vector>

namespace ni::random {

//! Strong wrapper for the Sobol dimension count. It exists so the dimension
//! constructor can never collide with the [rand.req.eng] result_type
//! (point-index) seed constructor -- necessary because result_type may itself
//! BE std::size_t for the 64-bit-width engine on LLP64 platforms (Windows).
//! Build one with the dimensions() helper for self-documenting call sites:
//! `Sobol64 e{dimensions(10)};`.
struct SobolDimensions {
  std::size_t value{1};
  constexpr explicit SobolDimensions(std::size_t d) noexcept : value(d) {}
};

//! Readable factory for SobolDimensions: `Sobol32 e{dimensions(5)};`.
[[nodiscard]] constexpr SobolDimensions dimensions(std::size_t d) noexcept {
  return SobolDimensions{d};
}

namespace detail {

//! The Sobol engine. State: the per-dimension direction integers m_v (fixed at
//! construction), the per-dimension accumulators m_x = X_j(point index), an
//! optional per-dimension digital-shift mask m_shift (default 0), the current
//! point index, and the scalar cursor (which coordinate is emitted next).
template <class UIntType = std::uint32_t, unsigned W = 32> class SobolEngine {
public:
  using result_type = UIntType;

  static_assert(std::is_unsigned_v<UIntType>,
                "Sobol result_type must be an unsigned integer");
  static_assert(W >= 1 && W <= sizeof(UIntType) * 8,
                "W must be in [1, bit width of UIntType]");

  static constexpr unsigned word_bits = W;
  static constexpr std::size_t default_dimension = 1;

  //! Default-construct a 1-dimensional generator positioned at the first
  //! emitted point (the origin is skipped by default).
  SobolEngine() : SobolEngine(SobolDimensions{default_dimension}) {}

  //! [rand.req.eng] value constructor. The argument is a STARTING POINT INDEX,
  //! not entropy: "seeding" a QRNG means choosing a position in the sequence.
  //! Dimension is 1 (use the SobolDimensions overloads for more).
  explicit SobolEngine(result_type startIndex)
      : SobolEngine(SobolDimensions{default_dimension}, startIndex) {}

  //! Construct a `dim`-dimensional generator positioned at the first emitted
  //! point. Throws std::out_of_range if dim exceeds the embedded table.
  explicit SobolEngine(SobolDimensions dim) {
    init(dim.value);
    seed();
  }

  //! Construct a `dim`-dimensional generator positioned at point `startIndex`.
  SobolEngine(SobolDimensions dim, result_type startIndex) {
    init(dim.value);
    seedIndex(static_cast<std::uint64_t>(startIndex));
  }

  //! [rand.req.eng] seed-sequence constructor: dimension 1, position derived
  //! from the sequence (see seed(Sseq&)).
  template <class Sseq>
    requires SeedSequence<Sseq>
  explicit SobolEngine(Sseq &q) {
    init(default_dimension);
    seed(q);
  }

  //! `dim`-dimensional generator positioned from a seed sequence.
  template <class Sseq>
    requires SeedSequence<Sseq>
  SobolEngine(SobolDimensions dim, Sseq &q) {
    init(dim.value);
    seed(q);
  }

  //! Reset to the first emitted point (index 1; the origin is skipped),
  //! preserving dimension and any digital shift. seed(0) instead positions at
  //! the origin.
  void seed() { seedIndex(originStartIndex); }

  //! Position at point index `startIndex`; reset the scalar cursor.
  void seed(result_type startIndex) {
    seedIndex(static_cast<std::uint64_t>(startIndex));
  }

  //! Derive a starting point index from the seed sequence (two 32-bit words
  //! merged into a 64-bit index), then position there. "Seed" = position.
  template <class Sseq>
    requires SeedSequence<Sseq>
  void seed(Sseq &q) {
    std::array<std::uint32_t, 2> words{};
    q.generate(words.begin(), words.end());
    const std::uint64_t index = static_cast<std::uint64_t>(words[0]) |
                                (static_cast<std::uint64_t>(words[1]) << 32);
    seedIndex(index);
  }

  //! Return the integer for the current coordinate, then advance the scalar
  //! cursor (and the point, after `dimension` coordinates). See the FOOTGUN
  //! note above.
  result_type operator()() {
    const result_type value = m_x[m_cursor] ^ m_shift[m_cursor];
    ++m_cursor;
    if (m_cursor >= m_dimension) {
      m_cursor = 0;
      advancePoint();
    }
    return value;
  }

  //! Advance the SCALAR stream by z coordinate emissions (composes correctly
  //! with the engine concept: discard(z) == z calls to operator()).
  void discard(unsigned long long z) {
    if (z == 0) {
      return;
    }
    const unsigned long long dim = m_dimension;
    const unsigned long long sum =
        static_cast<unsigned long long>(m_cursor) + z;
    m_index = maskIndex(m_index + sum / dim);
    m_cursor = static_cast<std::size_t>(sum % dim);
    computePointAccumulators(m_index);
  }

  //! Advance by `n` whole points, preserving the scalar cursor offset.
  void discard_points(unsigned long long n) {
    m_index = maskIndex(m_index + n);
    computePointAccumulators(m_index);
  }

  //! Return the current point's `dimension` coordinates in [0,1) (value / 2^W)
  //! and advance to the next point, resetting the scalar cursor. THIS is the
  //! low-discrepancy interface; prefer it over operator().
  [[nodiscard]] std::vector<double> next_point() {
    std::vector<double> point(m_dimension);
    fillCurrentPoint(point);
    advanceWholePoint();
    return point;
  }

  //! Fill `out` (size >= dimension()) with the current point's coordinates and
  //! advance to the next point. Only the first dimension() entries are written.
  void next_point(std::span<double> out) {
    fillCurrentPoint(out);
    advanceWholePoint();
  }

  //! Apply a random digital shift: an XOR mask per dimension drawn from `g`,
  //! XORed into every emitted integer (basic randomized QMC). A zero mask (the
  //! default) is a no-op. Owen scrambling is intentionally out of scope; this
  //! is the extension point.
  template <class URNG> void randomizeDigitalShift(URNG &g) {
    for (std::size_t j = 0; j < m_dimension; ++j) {
      m_shift[j] = drawWord(g);
    }
  }

  //! Clear any digital shift.
  void clearDigitalShift() {
    for (auto &s : m_shift) {
      s = 0;
    }
  }

  [[nodiscard]] std::size_t dimension() const noexcept { return m_dimension; }

  [[nodiscard]] static constexpr result_type min() noexcept { return 0; }

  [[nodiscard]] static constexpr result_type max() noexcept { return wmask; }

  [[nodiscard]] friend bool operator==(const SobolEngine &lhs,
                                       const SobolEngine &rhs) noexcept {
    // m_x is derived from m_index + m_v, so the index, cursor, dimension,
    // direction-number set, and shift fully determine the engine.
    return lhs.m_dimension == rhs.m_dimension && lhs.m_index == rhs.m_index &&
           lhs.m_cursor == rhs.m_cursor && lhs.m_v == rhs.m_v &&
           lhs.m_shift == rhs.m_shift;
  }

  [[nodiscard]] friend bool operator!=(const SobolEngine &lhs,
                                       const SobolEngine &rhs) noexcept {
    return !(lhs == rhs);
  }

  //! Round-trippable text serialization of the full state: dimension, point
  //! index, cursor, the per-dimension digital shift, then the d*W direction
  //! integers. Format flags and fill are forced to a known state and restored.
  template <class CharT, class Traits>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &os, const SobolEngine &e) {
    const auto flags = os.flags();
    const auto fill = os.fill();
    os.flags(std::ios_base::dec | std::ios_base::left);
    os.fill(os.widen(' '));
    const CharT space = os.widen(' ');
    os << e.m_dimension << space << e.m_index << space << e.m_cursor;
    for (std::size_t j = 0; j < e.m_dimension; ++j) {
      os << space << e.m_shift[j];
    }
    for (std::size_t j = 0; j < e.m_dimension; ++j) {
      for (unsigned k = 0; k < W; ++k) {
        os << space << e.m_v[j][k];
      }
    }
    os.flags(flags);
    os.fill(fill);
    return os;
  }

  template <class CharT, class Traits>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &is, SobolEngine &e) {
    const auto flags = is.flags();
    is.flags(std::ios_base::dec | std::ios_base::skipws);
    std::size_t dimension = 0;
    std::uint64_t index = 0;
    std::size_t cursor = 0;
    is >> dimension >> index >> cursor;
    std::vector<result_type> shift(dimension);
    for (auto &s : shift) {
      is >> s;
    }
    std::vector<std::array<result_type, W>> v(dimension);
    for (std::size_t j = 0; j < dimension; ++j) {
      for (unsigned k = 0; k < W; ++k) {
        is >> v[j][k];
      }
    }
    is.flags(flags);
    if (is) {
      e.m_dimension = dimension;
      e.m_index = index;
      e.m_cursor = cursor;
      e.m_shift = std::move(shift);
      e.m_v = std::move(v);
      e.m_x.assign(dimension, 0);
      e.computePointAccumulators(e.m_index);
    }
    return is;
  }

private:
  //! Low-W-bit mask for result words, guarded so the shift is never by the full
  //! width of result_type (which would be UB).
  static constexpr result_type wmask =
      (W >= static_cast<unsigned>(sizeof(result_type) * 8))
          ? static_cast<result_type>(~result_type(0))
          : static_cast<result_type>((result_type(1) << W) - 1);

  //! Point indices live mod 2^W (the sequence has period 2^W).
  static constexpr std::uint64_t indexMask =
      (W >= 64) ? ~std::uint64_t(0)
                : static_cast<std::uint64_t>((std::uint64_t(1) << W) - 1);

  //! Default first-emitted point index: 1, i.e. the origin (point 0) is
  //! skipped.
  static constexpr std::uint64_t originStartIndex = 1;

  static constexpr result_type maskWord(result_type v) noexcept {
    return v & wmask;
  }

  static constexpr std::uint64_t maskIndex(std::uint64_t i) noexcept {
    return i & indexMask;
  }

  void init(std::size_t dimension) {
    m_dimension = dimension;
    m_v = buildSobolDirectionNumbers<result_type, W>(
        dimension, defaultSobolDirectionTable());
    m_x.assign(dimension, 0);
    m_shift.assign(dimension, 0);
    m_index = 0;
    m_cursor = 0;
  }

  //! Position at point `index`, recomputing the accumulators directly from the
  //! Gray code (O(W*dimension)); reset the scalar cursor.
  void seedIndex(std::uint64_t index) {
    m_index = maskIndex(index);
    computePointAccumulators(m_index);
    m_cursor = 0;
  }

  //! X_j(index) = XOR of V_{k+1,j} over the set bits k of gray(index).
  void computePointAccumulators(std::uint64_t index) {
    const std::uint64_t gray = index ^ (index >> 1);
    for (std::size_t j = 0; j < m_dimension; ++j) {
      result_type acc = 0;
      for (unsigned b = 0; b < W; ++b) {
        if (((gray >> b) & std::uint64_t(1)) != 0) {
          acc ^= m_v[j][b];
        }
      }
      m_x[j] = acc;
    }
  }

  //! Advance to the next point. Uses the O(dimension) Antonov-Saleev update
  //! (flip the direction number at the lowest zero bit of the current index);
  //! falls back to a direct recompute on the period wrap.
  void advancePoint() {
    const std::uint64_t previous = m_index;
    m_index = maskIndex(m_index + 1);
    const unsigned b = static_cast<unsigned>(std::countr_one(previous));
    if (b < W) {
      for (std::size_t j = 0; j < m_dimension; ++j) {
        m_x[j] ^= m_v[j][b];
      }
    } else {
      computePointAccumulators(m_index);
    }
  }

  void advanceWholePoint() {
    m_cursor = 0;
    advancePoint();
  }

  //! coordinate = (X_j ^ shift_j) / 2^W.
  void fillCurrentPoint(std::span<double> out) const {
    for (std::size_t j = 0; j < m_dimension; ++j) {
      const result_type value = m_x[j] ^ m_shift[j];
      out[j] = std::ldexp(static_cast<double>(value), -static_cast<int>(W));
    }
  }

  //! Draw a W-bit word from a uniform random bit generator (for the digital
  //! shift), assembling 32 bits at a time so any URNG width works.
  template <class URNG> static result_type drawWord(URNG &g) {
    result_type result = 0;
    for (unsigned bit = 0; bit < W; bit += 32) {
      result =
          static_cast<result_type>((static_cast<std::uint64_t>(result) << 32) |
                                   static_cast<std::uint32_t>(g()));
    }
    return maskWord(result);
  }

  std::size_t m_dimension{0};
  std::vector<std::array<result_type, W>> m_v{};
  std::vector<result_type> m_x{};
  std::vector<result_type> m_shift{};
  std::uint64_t m_index{0};
  std::size_t m_cursor{0};
};

} // namespace detail
} // namespace ni::random
