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
//!   - the single `chacha_core()` member (selected per translation unit by a
//!     bare `#ifdef __SSE2__`) is refactored into three selectable block
//!     policies, `ScalarBlock` / `Sse2Block` / `NeonBlock`, so a forced-scalar
//!     engine and the native-SIMD engine can coexist in one TU and be compared
//!     at runtime;
//!   - an ARM NEON block implementation (`NeonBlock`) is added — original
//!     ni-random code, written from the ChaCha quarter-round, not present in
//!     the original gist;
//!   - the original AMD XOP micro-optimization path is dropped (dead silicon);
//!     only the portable scalar, SSE2, and SSSE3-refined rotate paths are kept.
//! The engine-level conformance additions (default constructor, no-arg seed(),
//! operator==) live in detail/ChaChaEngine.hpp, which carries the same notice.

//! \file ChaChaBlock.hpp
//!
//! The ChaCha block function (the R-round core that mixes the 16-word state),
//! provided as three interchangeable policies that produce bit-identical
//! output: a portable scalar reference, an x86 SSE2/SSSE3 path, and an ARM NEON
//! path. detail/ChaChaEngine.hpp parameterizes ChaChaImpl<R, Block> on which
//! one it uses; DefaultBlock selects the native SIMD policy where available and
//! the scalar policy otherwise. See plans/import-chacha-engine.md.
//!
//! Each policy exposes one operation:
//!
//!     template <std::size_t R> static void core(std::uint32_t* block)
//!     noexcept;
//!
//! which transforms the 16 input words in `block` in place by R ChaCha rounds
//! (R must be even). It performs only the round mixing — the caller is
//! responsible for the ChaCha feed-forward add of the original input. `block`
//! must point at 16 contiguous std::uint32_t and, for the SSE2 path, must be
//! 16-byte aligned (the engine stores it `alignas(16)`).

#pragma once

#include <cstddef>
#include <cstdint>

namespace ni::random::detail {

//! Portable scalar reference path. Compiles on every platform; defines the
//! sequence the SIMD paths must reproduce bit-for-bit.
struct ScalarBlock {
  [[nodiscard]] static constexpr std::uint32_t rotl(std::uint32_t x,
                                                    int n) noexcept {
    return (x << n) | (x >> (32 - n));
  }

  template <std::size_t R> static void core(std::uint32_t *x) noexcept {
    static_assert(R % 2 == 0, "ChaCha round count must be even");
    const auto quarterRound = [](std::uint32_t *s, int a, int b, int c, int d) {
      s[a] += s[b];
      s[d] ^= s[a];
      s[d] = rotl(s[d], 16);
      s[c] += s[d];
      s[b] ^= s[c];
      s[b] = rotl(s[b], 12);
      s[a] += s[b];
      s[d] ^= s[a];
      s[d] = rotl(s[d], 8);
      s[c] += s[d];
      s[b] ^= s[c];
      s[b] = rotl(s[b], 7);
    };
    for (std::size_t i = 0; i < R; i += 2) {
      quarterRound(x, 0, 4, 8, 12);
      quarterRound(x, 1, 5, 9, 13);
      quarterRound(x, 2, 6, 10, 14);
      quarterRound(x, 3, 7, 11, 15);
      quarterRound(x, 0, 5, 10, 15);
      quarterRound(x, 1, 6, 11, 12);
      quarterRound(x, 2, 7, 8, 13);
      quarterRound(x, 3, 4, 9, 14);
    }
  }
};

#if defined(__SSE2__)
#include <emmintrin.h>
#if defined(__SSSE3__)
#include <tmmintrin.h>
#endif

// 32-bit left-rotate of each lane by c. On SSSE3 the 8/16/24 rotates become a
// single byte shuffle (the original gist's _mm_roti_epi32); otherwise it is a
// shift/shift/xor. The XOP intrinsic path from the gist is dropped.
#if defined(__SSSE3__)
#define NI_RANDOM_CHACHA_ROTI(r, c)                                            \
  (((c) == 8)                                                                  \
       ? _mm_shuffle_epi8((r), _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6,   \
                                            5, 4, 7, 2, 1, 0, 3))              \
   : ((c) == 16)                                                               \
       ? _mm_shuffle_epi8((r), _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5,   \
                                            4, 7, 6, 1, 0, 3, 2))              \
   : ((c) == 24)                                                               \
       ? _mm_shuffle_epi8((r), _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4,   \
                                            7, 6, 5, 0, 3, 2, 1))              \
       : _mm_xor_si128(_mm_slli_epi32((r), (c)),                               \
                       _mm_srli_epi32((r), 32 - (c))))
#else
#define NI_RANDOM_CHACHA_ROTI(r, c)                                            \
  _mm_xor_si128(_mm_slli_epi32((r), (c)), _mm_srli_epi32((r), 32 - (c)))
#endif

//! x86 SSE2 (with an SSSE3 rotate refinement) path. Imported from the gist's
//! __SSE2__ chacha_core, reformatted; the diagonal-round lane rotations use
//! _mm_shuffle_epi32, matching ScalarBlock's index permutation.
struct Sse2Block {
  template <std::size_t R> static void core(std::uint32_t *block) noexcept {
    static_assert(R % 2 == 0, "ChaCha round count must be even");
    // ROTVn rotates the lanes of the given vector n places to the left.
#define NI_RANDOM_CHACHA_ROTV1(x) _mm_shuffle_epi32((x), 0x39)
#define NI_RANDOM_CHACHA_ROTV2(x) _mm_shuffle_epi32((x), 0x4e)
#define NI_RANDOM_CHACHA_ROTV3(x) _mm_shuffle_epi32((x), 0x93)

    __m128i a = _mm_load_si128(reinterpret_cast<const __m128i *>(block));
    __m128i b = _mm_load_si128(reinterpret_cast<const __m128i *>(block + 4));
    __m128i c = _mm_load_si128(reinterpret_cast<const __m128i *>(block + 8));
    __m128i d = _mm_load_si128(reinterpret_cast<const __m128i *>(block + 12));

    for (std::size_t i = 0; i < R; i += 2) {
      a = _mm_add_epi32(a, b);
      d = _mm_xor_si128(d, a);
      d = NI_RANDOM_CHACHA_ROTI(d, 16);
      c = _mm_add_epi32(c, d);
      b = _mm_xor_si128(b, c);
      b = NI_RANDOM_CHACHA_ROTI(b, 12);
      a = _mm_add_epi32(a, b);
      d = _mm_xor_si128(d, a);
      d = NI_RANDOM_CHACHA_ROTI(d, 8);
      c = _mm_add_epi32(c, d);
      b = _mm_xor_si128(b, c);
      b = NI_RANDOM_CHACHA_ROTI(b, 7);

      b = NI_RANDOM_CHACHA_ROTV1(b);
      c = NI_RANDOM_CHACHA_ROTV2(c);
      d = NI_RANDOM_CHACHA_ROTV3(d);

      a = _mm_add_epi32(a, b);
      d = _mm_xor_si128(d, a);
      d = NI_RANDOM_CHACHA_ROTI(d, 16);
      c = _mm_add_epi32(c, d);
      b = _mm_xor_si128(b, c);
      b = NI_RANDOM_CHACHA_ROTI(b, 12);
      a = _mm_add_epi32(a, b);
      d = _mm_xor_si128(d, a);
      d = NI_RANDOM_CHACHA_ROTI(d, 8);
      c = _mm_add_epi32(c, d);
      b = _mm_xor_si128(b, c);
      b = NI_RANDOM_CHACHA_ROTI(b, 7);

      b = NI_RANDOM_CHACHA_ROTV3(b);
      c = NI_RANDOM_CHACHA_ROTV2(c);
      d = NI_RANDOM_CHACHA_ROTV1(d);
    }

    _mm_store_si128(reinterpret_cast<__m128i *>(block), a);
    _mm_store_si128(reinterpret_cast<__m128i *>(block + 4), b);
    _mm_store_si128(reinterpret_cast<__m128i *>(block + 8), c);
    _mm_store_si128(reinterpret_cast<__m128i *>(block + 12), d);

#undef NI_RANDOM_CHACHA_ROTV3
#undef NI_RANDOM_CHACHA_ROTV2
#undef NI_RANDOM_CHACHA_ROTV1
  }
};

#undef NI_RANDOM_CHACHA_ROTI
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>

//! ARM NEON path. Original ni-random code (the gist has no NEON), written from
//! the ChaCha quarter-round and structured to mirror Sse2Block: the four state
//! rows live in four uint32x4_t lanes, the diffusion rotates are
//! shift-left/shift-right/or, and the diagonal-round lane permutations are
//! vextq_u32 (the NEON analogue of _mm_shuffle_epi32). Required to be
//! bit-identical to ScalarBlock — chacha_simd_equivalence_tests proves it.
struct NeonBlock {
  template <int N> [[nodiscard]] static uint32x4_t rotl(uint32x4_t v) noexcept {
    return vorrq_u32(vshlq_n_u32(v, N), vshrq_n_u32(v, 32 - N));
  }

  template <std::size_t R> static void core(std::uint32_t *block) noexcept {
    static_assert(R % 2 == 0, "ChaCha round count must be even");
    uint32x4_t a = vld1q_u32(block);
    uint32x4_t b = vld1q_u32(block + 4);
    uint32x4_t c = vld1q_u32(block + 8);
    uint32x4_t d = vld1q_u32(block + 12);

    for (std::size_t i = 0; i < R; i += 2) {
      a = vaddq_u32(a, b);
      d = veorq_u32(d, a);
      d = rotl<16>(d);
      c = vaddq_u32(c, d);
      b = veorq_u32(b, c);
      b = rotl<12>(b);
      a = vaddq_u32(a, b);
      d = veorq_u32(d, a);
      d = rotl<8>(d);
      c = vaddq_u32(c, d);
      b = veorq_u32(b, c);
      b = rotl<7>(b);

      b = vextq_u32(b, b, 1);
      c = vextq_u32(c, c, 2);
      d = vextq_u32(d, d, 3);

      a = vaddq_u32(a, b);
      d = veorq_u32(d, a);
      d = rotl<16>(d);
      c = vaddq_u32(c, d);
      b = veorq_u32(b, c);
      b = rotl<12>(b);
      a = vaddq_u32(a, b);
      d = veorq_u32(d, a);
      d = rotl<8>(d);
      c = vaddq_u32(c, d);
      b = veorq_u32(b, c);
      b = rotl<7>(b);

      b = vextq_u32(b, b, 3);
      c = vextq_u32(c, c, 2);
      d = vextq_u32(d, d, 1);
    }

    vst1q_u32(block, a);
    vst1q_u32(block + 4, b);
    vst1q_u32(block + 8, c);
    vst1q_u32(block + 12, d);
  }
};
#endif

//! The block policy the public ChaCha<R> uses: native SIMD where available,
//! scalar otherwise. NI_RANDOM_CHACHA_HAS_SIMD is 1 when DefaultBlock is a SIMD
//! policy (so the equivalence test knows there is a native path to compare
//! against) and 0 on a scalar-only platform.
#if defined(__ARM_NEON) || defined(__aarch64__)
using DefaultBlock = NeonBlock;
#define NI_RANDOM_CHACHA_HAS_SIMD 1
#elif defined(__SSE2__)
using DefaultBlock = Sse2Block;
#define NI_RANDOM_CHACHA_HAS_SIMD 1
#else
using DefaultBlock = ScalarBlock;
#define NI_RANDOM_CHACHA_HAS_SIMD 0
#endif

} // namespace ni::random::detail
