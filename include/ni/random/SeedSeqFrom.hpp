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
 * PCG Random Number Generation for C++
 *
 * Copyright 2014-2017 Melissa O'Neill <oneill@pcg-random.org>,
 *                     and the PCG Project contributors.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR MIT)
 *
 * Licensed under the Apache License, Version 2.0 (provided in
 * LICENSE-APACHE.txt and at http://www.apache.org/licenses/LICENSE-2.0)
 * or under the MIT license (provided in LICENSE-MIT.txt and at
 * http://opensource.org/licenses/MIT), at your option. This file may not
 * be copied, modified, or distributed except according to those terms.
 *
 * Distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied.  See your chosen license for details.
 *
 * For additional information about the PCG random number generation scheme,
 * visit http://www.pcg-random.org/.
 */
// clang-format on

//! \file SeedSeqFrom.hpp
//!
//! Verbatim import of pcg_extras::seed_seq_from from pcg-cpp
//! (pcg_extras.hpp), by Melissa O'Neill and the PCG Project contributors. The
//! body is unmodified; only the enclosing namespace (pcg_extras -> ni::random),
//! the public name (seed_seq_from -> SeedSeqFrom, per local PascalCase
//! convention), and the file organization differ from upstream.
//!
//! SeedSeqFrom is a *producer* / adapter, NOT a SeedSequence. It wraps an RNG
//! (e.g. std::random_device) so it can be used to seed an engine some other way
//! than from std::seed_seq. As upstream's own comment notes, it deliberately
//! does NOT meet the [rand.req.seedseq] requirements — it lacks the
//! rarely-used members (e.g. param()) — but engines only ever call generate(),
//! so in practice it works fine as an entropy source. The negative
//! static_assert below pins that producer-not-SeedSequence status so a future
//! change that accidentally makes it conform (or strips its members) fails
//! loudly.
//!
//! Coordination: plans/import-pcg-family.md imports the whole pcg_extras.hpp
//! into detail/. Whichever plan lands first owns the canonical seed_seq_from;
//! if that plan has already run, reuse its copy rather than defining it twice.

#pragma once
#include "ni/random/SeedSequence.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>

namespace ni::random {

//! Although std::seed_seq is useful, it isn't everything. Often we want to
//! initialize a random-number generator some other way, such as from a random
//! device. SeedSeqFrom adapts any RNG into such a seed source.
//!
//! Technically, it does not meet the requirements of a SeedSequence because it
//! lacks some of the rarely-used member functions (some of which would be
//! impossible to provide). However the C++ standard is quite specific that
//! actual engines only call the generate method, so it ought not to be a
//! problem in practice.
template <typename RngType> class SeedSeqFrom {
private:
  RngType rng_;

  typedef std::uint_least32_t result_type;

public:
  template <typename... Args>
  SeedSeqFrom(Args &&...args) : rng_(std::forward<Args>(args)...) {
    // Nothing (else) to do...
  }

  template <typename Iter> void generate(Iter start, Iter finish) {
    for (auto i = start; i != finish; ++i)
      *i = result_type(rng_());
  }

  constexpr std::size_t size() const {
    return (sizeof(typename RngType::result_type) > sizeof(result_type) &&
            RngType::max() > ~std::size_t(0UL))
               ? ~std::size_t(0UL)
               : std::size_t(RngType::max());
  }
};

//! SeedSeqFrom is intentionally a producer/adapter, not a [rand.req.seedseq]
//! seed sequence (no param(), no default construction, private result_type).
//! Pin the negative so the producer status is documented and a regression that
//! accidentally makes it conform fails loudly here.
static_assert(!SeedSequence<SeedSeqFrom<std::mt19937>>);

} // namespace ni::random
