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

//! \file Pcg64Unique.hpp
//!
//! Public per-engine header exposing the PCG `pcg64_unique` generator into
//! ni::random — the unique-stream XSL-RR variant with 128-bit state and 64-bit
//! output. The engine machinery and the Apache-2.0 OR MIT license notice live
//! in detail/PcgEngine.hpp (imported verbatim from pcg-cpp).
//! See plans/import-pcg-family.md.
//!
//! INTENTIONALLY NON-REPRODUCIBLE — read before using. The unique-stream mixin
//! derives the LCG increment from the engine object's own memory address on
//! every step and stores nothing (PcgEngine.hpp `unique_stream`). Consequences
//! that contradict [rand.req.eng] and this library's reproducibility contract:
//!   - a copy `auto f = e;` lives at a different address, so `f` produces a
//!     *different* sequence than `e`;
//!   - `operator==` is reflexive only — two distinct objects never compare
//!     equal, and a copy never equals its source;
//!   - stream round-trip works only in place — deserializing into a *new*
//!     object cannot restore the stream (it recomputes it from its address).
//! Because the RandomNumberEngine concept is purely structural it would *pass*
//! here, but it would then certify semantics this type does not honor. The
//! static_assert is therefore DELIBERATELY OMITTED, and the generic conformance
//! suite is not instantiated on this type. Its minimal tests (two instances
//! differ; in-place discard equivalence) live in test/pcg_unique_tests.cpp.

#pragma once
#include "ni/random/detail/PcgEngine.hpp"

namespace ni::random {
//! PCG XSL-RR 128/64, address-seeded unique stream — upstream `pcg64_unique`.
//! Non-reproducible by design; NOT asserted against RandomNumberEngine (see
//! the file-level note above).
using Pcg64Unique = pcg_engines::unique_xsl_rr_128_64;
} // namespace ni::random
