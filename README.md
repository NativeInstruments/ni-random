# ni-random

A header-only C++20 library of random number engines, distributions, and seeding
utilities, living in the `ni::random` namespace.

`ni-random` collects a broad set of well-regarded PRNG algorithms behind a single,
standard-shaped interface. Every engine models the C++ standard
`[rand.req.eng]` requirements and every distribution models `[rand.req.dist]`, so
they are drop-in compatible with `<random>` — an `ni::random` engine drives a
`std::` distribution, a `std::` engine drives an `ni::random` distribution, and
the pieces mix freely.

## Why this exists

The standard library guarantees the *interface* of `<random>` but not the
*algorithm*: `std::mt19937` is pinned, but `std::default_random_engine`,
`std::uniform_int_distribution`, etc. vary by implementation, so results are not
reproducible across platforms or toolchains. For audio DSP we need generators
that are fast, statistically sound, and bit-for-bit reproducible everywhere we
ship.

`ni-random` provides exactly that: a curated catalog of modern engines and
bias-free distributions whose output is fixed and verified by a conformance test
suite, with golden values pinned per engine to catch any algorithmic or seeding
drift.

## Design: concept + conformance suite

The library is built around one idea — a *contract* expressed in two layers:

- **A concept** (`RandomNumberEngine`, `RandomNumberDistribution`,
  `SeedSequence`) gates everything that is *structurally* checkable at compile
  time: the required members, signatures, and free operators. Each engine header
  ends with a `static_assert(RandomNumberEngine<...>)`, so a type that fails the
  contract fails to compile.
- **A conformance suite** verifies what a concept *cannot* — the semantic
  requirements: reproducibility (equal engines produce equal sequences),
  `discard(z)` equivalence, stream round-trip identity, and the pinned golden
  value. `std::minstd_rand` is used as the known-conforming oracle that every
  generic test must also pass.

This separation is deliberate and documented in the concept headers: concepts
check shape, the suite checks behavior.

## What's included

All engines are in `include/ni/random/<Engine>.hpp` (one header per engine);
distributions and utilities are siblings in the same directory.

### Engines

| Family | Engines |
|---|---|
| **xoshiro / xoroshiro** | `Xoshiro256Plus`, `Xoshiro256PlusPlus`, `Xoshiro256StarStar`, `Xoshiro128Plus`, `Xoshiro128PlusPlus`, `Xoshiro128StarStar`, `Xoroshiro128Plus`, `Xoroshiro128PlusPlus`, `Xoroshiro128StarStar` |
| **SplitMix** | `SplitMix64` (also used internally to seed the xoshiro family) |
| **PCG** | `Pcg32`, `Pcg64` and their `Fast` / `OneSeq` / `Unique` / `C32` / `C64` / `K32` / `K64` variants |
| **Philox** (counter-based) | `Philox4x32`, `Philox4x64` — match the C++26 predefined engines, implemented from the `[rand.eng.philox]` spec |
| **ChaCha** (counter-based, CSPRNG-grade) | `ChaCha` with `ChaCha8` / `ChaCha12` / `ChaCha20` rounds; scalar / SSE2 / NEON block policies |
| **Sobol** (quasi-random, low-discrepancy) | `Sobol32`, `Sobol64` — original implementations using Joe-Kuo direction numbers |

### Distributions

| Distribution | Notes |
|---|---|
| `UniformIntDistribution` | Bias-free (Lemire); `std::uniform_int_distribution` analogue |
| `UniformRealDistribution` | Optimized `generate_canonical`; `std::uniform_real_distribution` analogue |
| `NormalDistribution` | Marsaglia-polar; `std::normal_distribution` analogue |
| `ApproxNormalDistribution` | Extremely fast, intentionally imprecise normal approximation |
| `SimpleUniformIntDistribution`, `SimpleUniformRealDistribution` | Minimal, allocation-free uniform distributions |

### Seeding utilities

- `SeedPod` — thread-safe dispenser of sequential, non-zero seeds (first seed
  from `std::random_device`, then monotonically increasing; supports claiming).
- `SeedSeqFE` — fixed-entropy seed sequence (randutils).
- `SeedSeqFrom` — adapter that builds a seed sequence from a generator (pcg-cpp).

## Usage

The library is header-only — just include the engine and distribution you need.

```cpp
#include <ni/random/Xoshiro256PlusPlus.hpp>
#include <ni/random/UniformRealDistribution.hpp>

ni::random::Xoshiro256PlusPlus eng{12345};
ni::random::UniformRealDistribution<float> dist{-1.0f, 1.0f};

float sample = dist(eng);   // reproducible across platforms
```

Because every type satisfies the standard requirements, you can mix `ni::random`
and `std`:

```cpp
#include <random>
#include <ni/random/Pcg64.hpp>

ni::random::Pcg64 eng{std::random_device{}()};
std::normal_distribution<double> dist{0.0, 1.0};   // std distribution, ni engine
double x = dist(eng);
```

## Build & test

This is a sub-repo of the SUITES monorepo and uses the shared GYP/ninja/izdeps
build system (see the monorepo-root `CLAUDE.md`). Being header-only, the only
buildable artifact is the unit-test bundle.

Dependencies (`.izdeps.toml`): `izbuildconfig` (shared `.gypi` build config),
`izcore` (linked by the test target), and `google-testing`.

```bash
# from this directory
ggp --ninja --ninja-output-dir-override=../../out
ninja -C out/Debug ni-randomUnitTests
xcrun xctest out/Debug/ni-randomUnitTests.xctest   # macOS
```

`izo_build/ni-random_sources.gypi` lists every header and test file and must be
updated when adding or removing one. CI runs `build_scripts/ci.py`, which builds
the test target with sanitizers enabled and runs style checks.

### Adding an engine

1. Add `include/ni/random/<Engine>.hpp` ending in
   `static_assert(RandomNumberEngine<...>)`.
2. Add `test/<engine>_tests.cpp` that includes the engine, the shared
   `EngineConformanceSuite`, and the traits, then instantiates the
   type-parametrized suite with a pinned golden value.
3. List both new files in `izo_build/ni-random_sources.gypi`.

The conformance suite (`test/EngineConformanceSuite.hpp`) is generic and is never
edited per engine — all engine-specific values come from the per-engine traits.

## Formatting

There is **no `.clang-format`** in this repo: clang-format's built-in **LLVM
style** is the convention here (unlike the iZotope tab style used elsewhere in the
monorepo). Format with plain `clang-format -i` or `git clang-format origin/HEAD`.
`.clang-tidy` enforces a curated check set with `WarningsAsErrors`.

> **Note:** the org `format-staged` pre-commit hook re-tabs staged C++ back to the
> iZotope style, which silently undoes LLVM formatting. Commit with
> `git commit --no-verify` and verify the committed blob with
> `clang-format --dry-run -Werror`.

## Licensing

`ni-random` is released under the **BSD 3-Clause License** (see `LICENSE`).

It incorporates several permissively-licensed third-party components (the
xoshiro/xoroshiro family and SplitMix64 from Xoshiro-cpp, PCG from pcg-cpp,
ChaCha from Orson Peters, the UTL distributions, randutils' `seed_seq_fe`, and the
Joe-Kuo Sobol direction-number table). Each retains its original license, and all
are compatible with redistribution under BSD-3-Clause. The Philox and Sobol
*implementations* are original to `ni-random`, written from public specifications.
Per-component notices are aggregated in `THIRD-PARTY-LICENSES.md` and reproduced
verbatim in the relevant source files.
