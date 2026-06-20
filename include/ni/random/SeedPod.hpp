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

//! \file SeedPod.hpp
//!
//! ni::random::SeedPod — a thread-safe dispenser of sequential, non-zero PRNG
//! seeds. The first seed is chosen at random (std::random_device); every
//! subsequent NewSeed() hands out the next integer in the sequence. ClaimSeed
//! lets a caller reserve a value so that later NewSeed() results stay strictly
//! greater than it. A seed of zero is never returned, because zero is reserved
//! to mean "no seed saved to serialized state".
//!
//! SeedType is the unsigned integer type handed out as a seed; it defaults to
//! std::uint64_t, which matches the result_type of the 64-bit engines in this
//! library.

#include <atomic>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

namespace ni::random {

//! Manages a sequential list of PRNG seeds.
template <class SeedT = std::uint64_t> class SeedPod {
public:
  using SeedType = SeedT;
  static_assert(std::is_unsigned_v<SeedType>,
                "SeedPod seeds must be an unsigned integer type");

  //! Randomly generates the first seed in the sequence.
  SeedPod()
      : m_nextSeed{[] {
          // first time a seed is requested, randomly generate the first seed
          auto rd = std::random_device{};
          auto gen = std::mt19937{rd()};
          auto dist = std::uniform_int_distribution<SeedType>{
              1u, std::numeric_limits<SeedType>::max() / 3u};

          return dist(gen);
        }()} {}

  ~SeedPod() = default;

  SeedPod(const SeedPod &) = delete;
  SeedPod(SeedPod &&) = delete;

  SeedPod &operator=(const SeedPod &) = delete;
  SeedPod &operator=(SeedPod &&) = delete;

  //! Return the next non-zero sequential seed.
  SeedType NewSeed() {
    auto newSeed = m_nextSeed.fetch_add(1);
    //! Seed of zero is not allowed, because that is used to represent no seed
    //! saved to serialized state.
    if (newSeed == 0) {
      // Claim seed value 1 instead.
      ClaimSeed(1);
      return 1;
    }
    return newSeed;
  }

  //! Claim a seed; after calling `ClaimSeed(seed)`, `(NewSeed() > seed) ==
  //! true`.
  void ClaimSeed(SeedType seed) {
    auto nextSeed = m_nextSeed.load();
    while (seed >= nextSeed) {
      if (m_nextSeed.compare_exchange_weak(nextSeed, seed + 1)) {
        return;
      }
    }
  }

private:
  std::atomic<SeedType> m_nextSeed;
};

} // namespace ni::random
