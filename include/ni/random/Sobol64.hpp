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

//! \file Sobol64.hpp
//!
//! Public per-engine header exposing the 64-bit-width Sobol low-discrepancy
//! generator into ni::random as Sobol64. The engine core lives in
//! detail/SobolEngine.hpp and the direction numbers in
//! detail/SobolDirectionNumbers.hpp.
//!
//! Sobol is a quasi-random (low-discrepancy) generator: dimension is fixed at
//! construction and the scalar operator() walks one point's coordinates per
//! `dimension` calls. PREFER next_point() -- piping the scalar surface through
//! a <random> distribution destroys the low-discrepancy property. See the
//! FOOTGUN note in detail/SobolEngine.hpp and plans/qrng-sobol.md.
//!
//! Original ni-random code from the published Antonov-Saleev / Joe-Kuo
//! construction; no third-party source, hence no license notice.

#include "ni/random/RandomNumberEngine.hpp"
#include "ni/random/detail/SobolEngine.hpp"

#include <cstdint>

namespace ni::random {
//! Sobol generator with 64-bit direction numbers / coordinates.
using Sobol64 = detail::SobolEngine<std::uint64_t, 64>;
} // namespace ni::random

static_assert(ni::random::RandomNumberEngine<ni::random::Sobol64>);
static_assert(std::uniform_random_bit_generator<ni::random::Sobol64>);
