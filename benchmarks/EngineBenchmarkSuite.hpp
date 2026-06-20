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

//! \file EngineBenchmarkSuite.hpp
//!
//! Shared Google Benchmark scaffolding for the ni::random engines, mirroring
//! the test-side EngineConformanceSuite: one generic body, instantiated once
//! per engine from its own <engine>_benchmark.cpp via
//! NI_RANDOM_ENGINE_BENCHMARK.
//!
//! The single metric is raw generation throughput -- the canonical PRNG figure
//! of merit -- so every engine produces one comparable row. The benchmark
//! repeatedly invokes operator() and accumulates the result behind
//! benchmark::DoNotOptimize so the call cannot be elided, and reports both
//! items/s (numbers generated) and bytes/s (sizeof(result_type) per number) to
//! make engines with different output widths directly comparable.
//!
//! NOTE on Sobol: this measures the scalar operator() surface (one coordinate
//! per call), which is what the RandomNumberEngine concept exposes; it is not a
//! measure of next_point() quasi-random point generation.

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>

namespace ni::random::bench {

//! Raw generation throughput for a default-constructed Engine.
template <class Engine> void Generate(::benchmark::State &state) {
  Engine engine{};
  typename Engine::result_type acc = 0;
  for (auto _ : state) {
    acc += engine();
    ::benchmark::DoNotOptimize(acc);
  }
  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
  state.SetBytesProcessed(
      static_cast<std::int64_t>(state.iterations()) *
      static_cast<std::int64_t>(sizeof(typename Engine::result_type)));
}

} // namespace ni::random::bench

//! Registers the generation benchmark for ni::random::<Engine> under the plain
//! engine name (e.g. "Xoshiro256Plus"). One invocation per
//! <engine>_benchmark.cpp.
#define NI_RANDOM_ENGINE_BENCHMARK(Engine)                                     \
  [[maybe_unused]] static const auto *const NI_RANDOM_BENCH_##Engine =         \
      ::benchmark::RegisterBenchmark(                                          \
          #Engine, ::ni::random::bench::Generate<::ni::random::Engine>)
