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

//! \file ChaCha.hpp
//!
//! Public surface for the ChaCha stream-cipher random number engine. The engine
//! and its block-function policies are imported (with modifications) from Orson
//! Peters' zlib-licensed ChaCha generator; the verbatim license notice and the
//! "altered version" marking live in detail/ChaChaEngine.hpp and
//! detail/ChaChaBlock.hpp, which hold the copied algorithm. See
//! plans/import-chacha-engine.md.

#pragma once

#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/ChaChaEngine.hpp"

#include <cstddef>

namespace ni::random {

//! ChaCha stream-cipher RNG with R rounds (R even). The block function is
//! selected per platform — ARM NEON, x86 SSE2/SSSE3, or a portable scalar
//! fallback — all bit-identical. The public type is fixed to the native block
//! policy; the policy is an internal detail (detail::ChaChaImpl) the tests use
//! to compare scalar and SIMD paths. ChaCha(seed, stream) exposes independent
//! substreams via the optional stream argument.
template <std::size_t R> using ChaCha = detail::ChaChaImpl<R>;

//! ChaCha with the standard 20 rounds — the tested, conformance-gated alias.
using ChaCha20 = ChaCha<20>;

//! Reduced-round ChaCha variants. ChaCha8 / ChaCha12 trade diffusion for speed;
//! they share ChaCha20's interface, block policies, and substream support.
using ChaCha8 = ChaCha<8>;
using ChaCha12 = ChaCha<12>;

} // namespace ni::random

static_assert(ni::random::RandomNumberEngine<ni::random::ChaCha8>);
static_assert(ni::random::RandomNumberEngine<ni::random::ChaCha12>);
static_assert(ni::random::RandomNumberEngine<ni::random::ChaCha20>);
