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

//! \file PhiloxEngine.hpp
//!
//! The n = 4 Philox counter-based engine core, shared by the two predefined
//! ni::random engines (Philox4x32 / Philox4x64). Parameterized on the word
//! width w, the result_type, the round count r, and the four predefined
//! constants (the two multipliers M0/M1 and the two round constants C0/C1).
//!
//! This is an *implementation detail*, not a public general philox_engine
//! template: only the two predefined 4xW aliases are exported (see
//! Philox4x32.hpp / Philox4x64.hpp). n is fixed at 4 and the constants are
//! supplied by those aliases.
//!
//! Original ni-random code implemented directly from the C++26
//! [rand.eng.philox] specification -- no third-party (e.g. random123) source is
//! copied, adapted, or included, so there is no license notice to reproduce.
//! See plans/add-philox-engines.md.

#include "ni/random/SeedSequence.hpp"
#include "ni/random/detail/PhiloxMath.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

namespace ni::random::detail {

//! The Philox engine for n = 4. The counter X (4 words), key K (2 words),
//! output buffer Y (4 words), and index i make up the [rand.eng.philox] state.
//! All arithmetic and stored words are masked to the low w bits, so the engine
//! reproduces the standard's sequence even when result_type is wider than w
//! (e.g. a 64-bit uint_fast32_t).
template <class UIntType, int W, std::size_t R, UIntType M0, UIntType C0,
          UIntType M1, UIntType C1>
class PhiloxEngine {
public:
  using result_type = UIntType;

  static constexpr std::size_t word_count = 4; // n
  static constexpr std::size_t key_count = 2;  // n / 2
  static constexpr std::size_t round_count = R;
  static constexpr int word_bits = W;

  //! [rand.predef]: the predefined philox engines use this default seed.
  static constexpr result_type default_seed = 20111115u;

  PhiloxEngine() : PhiloxEngine(default_seed) {}

  explicit PhiloxEngine(result_type value) { seed(value); }

  template <class Sseq>
    requires SeedSequence<Sseq>
  explicit PhiloxEngine(Sseq &q) {
    seed(q);
  }

  //! [rand.eng.philox] value seeding: K0 = value mod 2^w; all other K and all
  //! X are 0; i = n - 1.
  void seed(result_type value = default_seed) {
    m_k.fill(0);
    m_k[0] = mask(value);
    m_x.fill(0);
    m_y.fill(0);
    m_i = word_count - 1;
  }

  //! [rand.eng.philox] seed-sequence seeding: with p = ceil(w / 32), generate
  //! (n/2) * p 32-bit words and assemble each key Kk from its p words mod 2^w;
  //! all X are 0; i = n - 1.
  template <class Sseq>
    requires SeedSequence<Sseq>
  void seed(Sseq &q) {
    constexpr std::size_t p = (W + 31) / 32;
    std::array<std::uint32_t, key_count * p> a{};
    q.generate(a.begin(), a.end());
    for (std::size_t k = 0; k < key_count; ++k) {
      result_type kk = 0;
      for (std::size_t j = 0; j < p; ++j) {
        kk |= static_cast<result_type>(a[k * p + j]) << (32 * j);
      }
      m_k[k] = mask(kk);
    }
    m_x.fill(0);
    m_y.fill(0);
    m_i = word_count - 1;
  }

  void discard(unsigned long long z) {
    for (unsigned long long n = 0; n < z; ++n) {
      (*this)();
    }
  }

  //! [rand.eng.philox] transition + generation: advance i; on wrap, refill Y
  //! from the current counter, then increment the counter; return Y[i].
  result_type operator()() {
    ++m_i;
    if (m_i == word_count) {
      m_y = philoxFunction(m_k, m_x);
      incrementCounter();
      m_i = 0;
    }
    return m_y[m_i];
  }

  //! [rand.eng.philox] set_counter: X_j = c_{n-1-j} mod 2^w; i = n - 1. Used to
  //! position the engine at an arbitrary counter for parallel substreams; the
  //! next n outputs are the Philox(K, X) batch for the counter just set.
  void set_counter(const std::array<result_type, word_count> &c) {
    for (std::size_t j = 0; j < word_count; ++j) {
      m_x[j] = mask(c[word_count - 1 - j]);
    }
    m_y.fill(0);
    m_i = word_count - 1;
  }

  [[nodiscard]] static constexpr result_type min() noexcept { return 0; }

  [[nodiscard]] static constexpr result_type max() noexcept { return wmask; }

  [[nodiscard]] friend bool operator==(const PhiloxEngine &lhs,
                                       const PhiloxEngine &rhs) noexcept {
    // Y is a derived cache (Y == Philox(K, X - 1) whenever i < n - 1, and is
    // unread at i == n - 1), so the key, counter, and index fully determine the
    // engine; comparing them avoids spurious inequality from a stale buffer.
    return lhs.m_k == rhs.m_k && lhs.m_x == rhs.m_x && lhs.m_i == rhs.m_i;
  }

  //! [rand.eng.philox] textual representation: K0..K(n/2-1), X0..X(n-1), i, in
  //! that order. Y is not streamed -- it is reconstructed from K and X on
  //! extraction. Format flags and fill are forced to a known state and restored
  //! so insertion does not perturb the stream.
  template <class CharT, class Traits>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &os, const PhiloxEngine &e) {
    const auto flags = os.flags();
    const auto fill = os.fill();
    os.flags(std::ios_base::dec | std::ios_base::left);
    os.fill(os.widen(' '));
    const CharT space = os.widen(' ');
    for (std::size_t k = 0; k < key_count; ++k) {
      os << e.m_k[k] << space;
    }
    for (std::size_t j = 0; j < word_count; ++j) {
      os << e.m_x[j] << space;
    }
    os << e.m_i;
    os.flags(flags);
    os.fill(fill);
    return os;
  }

  template <class CharT, class Traits>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &is, PhiloxEngine &e) {
    const auto flags = is.flags();
    is.flags(std::ios_base::dec | std::ios_base::skipws);
    std::array<result_type, key_count> k{};
    std::array<result_type, word_count> x{};
    std::size_t i = 0;
    for (auto &kk : k) {
      is >> kk;
    }
    for (auto &xx : x) {
      is >> xx;
    }
    is >> i;
    is.flags(flags);
    if (is) {
      e.m_k = k;
      e.m_x = x;
      e.m_i = i;
      e.reconstructBuffer();
    }
    return is;
  }

private:
  //! Low-w-bit mask. Computed with a width guard so the shift is never by the
  //! full width of result_type (which would be UB).
  static constexpr result_type wmask =
      (W >= static_cast<int>(sizeof(result_type) * 8))
          ? static_cast<result_type>(~result_type(0))
          : static_cast<result_type>((result_type(1) << W) - 1);

  static constexpr result_type mask(result_type v) noexcept {
    return v & wmask;
  }

  //! The Philox(K, X) bijection: r rounds, each permuting the state per the
  //! n = 4 table f = {2, 1, 0, 3} then applying the round function with the
  //! round-bumped keys keyq_k = (K_k + q * C_k) mod 2^w.
  static std::array<result_type, word_count>
  philoxFunction(const std::array<result_type, key_count> &key,
                 const std::array<result_type, word_count> &counter) {
    std::array<result_type, word_count> x = counter;
    for (std::size_t q = 0; q < round_count; ++q) {
      const std::array<result_type, word_count> v = {x[2], x[1], x[0], x[3]};
      const result_type keyq0 = mask(key[0] + static_cast<result_type>(q) * C0);
      const result_type keyq1 = mask(key[1] + static_cast<result_type>(q) * C1);
      result_type hi0 = 0;
      const result_type lo0 = mulhilo<W>(v[0], M0, hi0);
      result_type hi1 = 0;
      const result_type lo1 = mulhilo<W>(v[2], M1, hi1);
      x[0] = mask(hi0 ^ keyq0 ^ v[1]);
      x[1] = lo0;
      x[2] = mask(hi1 ^ keyq1 ^ v[3]);
      x[3] = lo1;
    }
    return x;
  }

  //! Add 1 to X as a single n*w-bit little-endian integer (X0 least
  //! significant), carrying across words mod 2^w.
  void incrementCounter() noexcept {
    for (std::size_t j = 0; j < word_count; ++j) {
      m_x[j] = mask(m_x[j] + 1);
      if (m_x[j] != 0) {
        break;
      }
    }
  }

  //! Subtract 1 from a counter as a single n*w-bit little-endian integer,
  //! borrowing across words. Used to recover the batch counter on extraction.
  static void
  decrementCounter(std::array<result_type, word_count> &x) noexcept {
    for (std::size_t j = 0; j < word_count; ++j) {
      if (x[j] != 0) {
        x[j] = mask(x[j] - 1);
        break;
      }
      x[j] = wmask;
    }
  }

  //! Recompute Y after K/X/i are restored from a stream. The counter is one
  //! batch ahead of the buffer being dispensed, so Y = Philox(K, X - 1) while a
  //! batch is in flight (i < n - 1); at i == n - 1 the next call refills Y, so
  //! its value is irrelevant.
  void reconstructBuffer() {
    if (m_i != word_count - 1) {
      std::array<result_type, word_count> prev = m_x;
      decrementCounter(prev);
      m_y = philoxFunction(m_k, prev);
    } else {
      m_y.fill(0);
    }
  }

  std::array<result_type, key_count> m_k{};
  std::array<result_type, word_count> m_x{};
  std::array<result_type, word_count> m_y{};
  std::size_t m_i{0};
};

} // namespace ni::random::detail
