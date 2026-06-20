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
/*
    Copyright (c) 2024 Orson Peters <orsonpeters@gmail.com>

    This software is provided 'as-is', without any express or implied warranty. In no event will the
    authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose, including commercial
    applications, and to alter it and redistribute it freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the
       original software. If you use this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as
       being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
// clang-format on

//! ALTERED VERSION NOTICE (zlib clause 2): this file is a version of Orson
//! Peters' ChaCha generator (https://gist.github.com/orlp/32f5d1b631ab092608b1)
//! modified by Native Instruments. The changes relative to the original are:
//!   - the block function is factored out into the selectable policies in
//!     detail/ChaChaBlock.hpp and parameterized as the Block template argument
//!     (so a forced-scalar engine and the native-SIMD engine can coexist in one
//!     translation unit and be compared at runtime); the engine here keeps only
//!     the input assembly and the ChaCha feed-forward add in generateBlock();
//!   - a default constructor and a no-argument seed() are added (via a
//!     documented ChaChaDefaultSeed) so the type models
//!     std::default_initializable and the [rand.req.eng] seed() row;
//!   - the unconstrained template seed-sequence constructor / seed() are
//!     constrained with ni::random::SeedSequence so a std::uint64_t lvalue
//!     unambiguously selects the value-seed overload;
//!   - operator!= is added and operator== is kept (it already compared the full
//!     logical state — keysetup + ctr — which is exactly what the stream
//!     operators serialize); the stream extraction sets dec | skipws (the
//!     original cleared skipws, which would have stalled multi-token reads).
//! The block-function refactor, the original ARM NEON block, and the dropped
//! XOP path are described in detail/ChaChaBlock.hpp, which carries the same
//! notice.

//! \file ChaChaEngine.hpp
//!
//! The ChaCha stream-cipher random number engine, ChaChaImpl<R, Block>, adapted
//! from Orson Peters' zlib ChaCha generator and made conformant with
//! ni::random::RandomNumberEngine. R is the (even) round count; Block is the
//! block-function policy from detail/ChaChaBlock.hpp (DefaultBlock by default —
//! native SIMD where available, scalar otherwise). The public surface is
//! ni::random::ChaCha<R> / ChaCha20 (ChaCha.hpp); the Block parameter is an
//! internal detail used by the tests to instantiate forced-scalar and
//! native-SIMD engines side by side. See plans/import-chacha-engine.md.

#pragma once

#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/ChaChaBlock.hpp"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

namespace ni::random::detail {

//! Default seed for a default-constructed ChaCha (with stream 0). The pinned
//! golden conformance value is the 10000th output of a ChaCha20 seeded with it.
//! Same numeric value as the library-wide ni::random::DefaultSeed, kept local
//! here so this engine does not depend on the xoshiro detail header.
inline constexpr std::uint64_t ChaChaDefaultSeed = 1234567890ULL;

template <std::size_t R, class Block = DefaultBlock> class ChaChaImpl {
public:
  using result_type = std::uint32_t;

  explicit ChaChaImpl(std::uint64_t seedval = ChaChaDefaultSeed,
                      std::uint64_t stream = 0) noexcept {
    seed(seedval, stream);
  }

  template <class Sseq>
    requires SeedSequence<Sseq>
  explicit ChaChaImpl(Sseq &seq) {
    seed(seq);
  }

  //! Value seed. Default arguments give both the no-argument seed() the concept
  //! requires (reset to ChaChaDefaultSeed, stream 0) and the single-value
  //! seed(s) row. stream selects an independent ChaCha substream.
  void seed(std::uint64_t seedval = ChaChaDefaultSeed,
            std::uint64_t stream = 0) noexcept {
    m_ctr = 0;
    m_blockIdx = std::uint64_t(-1); // Block is assumed to be uninitialized.
    m_keysetup[0] = seedval & 0xffffffffu;
    m_keysetup[1] = static_cast<std::uint32_t>(seedval >> 32);
    m_keysetup[2] = m_keysetup[3] = 0xdeadbeefu; // Could use 128-bit seed.
    m_keysetup[4] = stream & 0xffffffffu;
    m_keysetup[5] = static_cast<std::uint32_t>(stream >> 32);
    m_keysetup[6] = m_keysetup[7] = 0xdeadbeefu; // Could use 128-bit stream.
  }

  template <class Sseq>
    requires SeedSequence<Sseq>
  void seed(Sseq &seq) {
    m_ctr = 0;
    m_blockIdx = std::uint64_t(-1); // Block is assumed to be uninitialized.
    seq.generate(m_keysetup, m_keysetup + 8);
  }

  result_type operator()() noexcept {
    const std::uint64_t nextBlockIdx = m_ctr / 16;
    const std::uint64_t idxInBlock = m_ctr % 16;
    if (nextBlockIdx != m_blockIdx) {
      m_blockIdx = nextBlockIdx;
      generateBlock();
    }
    ++m_ctr;
    return m_block[idxInBlock];
  }

  void discard(unsigned long long n) noexcept { m_ctr += n; }

  [[nodiscard]] static constexpr result_type min() noexcept { return 0u; }

  [[nodiscard]] static constexpr result_type max() noexcept {
    return 0xffffffffu;
  }

  //! Equality over the complete logical state: keysetup and ctr. The block
  //! buffer and block_idx are a cache derived from these, so two engines equal
  //! here produce identical sequences. This is exactly the state the stream
  //! operators serialize, keeping == consistent with stream round-trip.
  [[nodiscard]] friend bool operator==(const ChaChaImpl &lhs,
                                       const ChaChaImpl &rhs) noexcept {
    for (std::size_t i = 0; i < 8; ++i) {
      if (lhs.m_keysetup[i] != rhs.m_keysetup[i]) {
        return false;
      }
    }
    return lhs.m_ctr == rhs.m_ctr;
  }

  [[nodiscard]] friend bool operator!=(const ChaChaImpl &lhs,
                                       const ChaChaImpl &rhs) noexcept {
    return !(lhs == rhs);
  }

  //! Serializes keysetup and ctr as space-separated decimal integers. Format
  //! flags and fill are forced to a known state and restored so insertion does
  //! not perturb the stream. CharT-generic (wchar_t round-trips for free).
  template <class CharT, class Traits>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &os, const ChaChaImpl &rng) {
    const auto flags = os.flags();
    const auto fill = os.fill();
    const CharT space = os.widen(' ');
    os.flags(std::ios_base::dec | std::ios_base::left);
    os.fill(space);
    for (std::size_t i = 0; i < 8; ++i) {
      os << rng.m_keysetup[i] << space;
    }
    os << rng.m_ctr;
    os.flags(flags);
    os.fill(fill);
    return os;
  }

  template <class CharT, class Traits>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &is, ChaChaImpl &rng) {
    const auto flags = is.flags();
    is.flags(std::ios_base::dec | std::ios_base::skipws);
    for (std::size_t i = 0; i < 8; ++i) {
      is >> rng.m_keysetup[i];
    }
    is >> rng.m_ctr;
    rng.m_blockIdx = std::uint64_t(-1); // Block is assumed to be uninitialized.
    is.flags(flags);
    return is;
  }

private:
  //! Assemble the 16-word ChaCha input (constants ‖ keysetup ‖ counter), run R
  //! rounds via the Block policy, then the ChaCha feed-forward add of the
  //! original input. The round mixing lives in Block::core; everything else
  //! (the input layout and the add) stays here so all three policies are
  //! drop-in.
  void generateBlock() noexcept {
    const std::uint32_t constants[4] = {0x61707865u, 0x3320646eu, 0x79622d32u,
                                        0x6b206574u};
    std::uint32_t input[16];
    for (std::size_t i = 0; i < 4; ++i) {
      input[i] = constants[i];
    }
    for (std::size_t i = 0; i < 8; ++i) {
      input[4 + i] = m_keysetup[i];
    }
    input[12] = m_blockIdx & 0xffffffffu;
    input[13] = static_cast<std::uint32_t>(m_blockIdx >> 32);
    input[14] = input[15] = 0xdeadbeefu; // Could use 128-bit counter.

    for (std::size_t i = 0; i < 16; ++i) {
      m_block[i] = input[i];
    }
    Block::template core<R>(m_block);
    for (std::size_t i = 0; i < 16; ++i) {
      m_block[i] += input[i];
    }
  }

  alignas(16) std::uint32_t m_block[16] = {};
  std::uint64_t m_blockIdx = 0;
  std::uint32_t m_keysetup[8] = {};
  std::uint64_t m_ctr = 0;
};

} // namespace ni::random::detail
