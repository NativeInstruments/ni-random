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

//! \file Pcg64.hpp
//!
//! Public per-engine header exposing the PCG `pcg64` generator into
//! ni::random — the setseq XSL-RR variant with 128-bit state and 64-bit
//! output. The engine machinery and the Apache-2.0 OR MIT license notice live
//! in detail/PcgEngine.hpp (imported verbatim from pcg-cpp).
//! See plans/import-pcg-family.md.
//!
//! PERFORMANCE CAVEAT (Windows): `pcg64` carries 128-bit state. On Clang/GCC
//! that is the native `__uint128_t`; on MSVC `__SIZEOF_INT128__` is undefined,
//! so it falls back to the software `pcg_extras::uint_x4` math in
//! detail/PcgUint128.hpp and is meaningfully slower. For hot real-time DSP
//! paths on Windows prefer Pcg32 (native 64-bit state everywhere).

#pragma once
#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/PcgEngine.hpp"

namespace ni::random {
//! PCG XSL-RR 128/64, settable stream — upstream `pcg64`.
using Pcg64 = pcg_engines::setseq_xsl_rr_128_64;
} // namespace ni::random

static_assert(ni::random::RandomNumberEngine<ni::random::Pcg64>);
