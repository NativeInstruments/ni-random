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

//! \file Pcg64C32Fast.hpp
//!
//! Public per-engine header exposing the PCG `pcg64_c32_fast` extended
//! generator into ni::random — the c-variant extended XSL-RR 128/64 on an MCG
//! base (kdd=false). The engine machinery and the Apache-2.0 OR MIT license
//! notice live in detail/PcgEngine.hpp (imported verbatim from pcg-cpp).
//! See plans/import-pcg-family.md.
//!
//! PERFORMANCE CAVEAT (Windows): 128-bit state uses software math on MSVC (see
//! detail/PcgUint128.hpp) and is slower than on Clang/GCC. Prefer a pcg32
//! variant for hot real-time DSP paths on Windows.

#pragma once
#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/PcgEngine.hpp"

namespace ni::random {
//! PCG XSL-RR 128/64 extended, c-variant on MCG base — upstream
//! `pcg64_c32_fast`.
using Pcg64C32Fast = pcg_engines::ext_mcg_xsl_rr_128_64<5, 128, false>;
} // namespace ni::random

static_assert(ni::random::RandomNumberEngine<ni::random::Pcg64C32Fast>);
