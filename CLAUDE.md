# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

`ni-random` is a sub-repo of the SUITES monorepo. The monorepo-root
`../CLAUDE.md` covers the shared GYP/ninja/izdeps build system, branching model,
formatting rules, and C++ naming conventions — all of which apply here. This file
adds only what is specific to `ni-random`.

## What this repo is

Two things at once:

1. A **header-only C++20 library of random number engines** in the `ni::random`
   namespace (`include/ni/random/`). No `.cpp` library sources — the `ni-random`
   GYP target is `type: 'none'` and exports `include/` via `all_dependent_settings`.
2. The **skeleton/template for a frameworks repo** (see `README.md`). It also
   houses the *canonical* `.clang-tidy` and `.gitignore`. If you change either,
   the README requires copying the new version to the Suites Boilerplate and DSP
   Boilerplate repos.

Dependencies (`.izdeps.toml`): `izbuildconfig` (shared `.gypi` build config),
`izcore` (`iZCoreCommon`, linked only by the test target), and `google-testing`.

## Build & test

Header-only, so the only buildable artifact is the unit-test bundle. From this
directory (depth requires `../../out` per the root CLAUDE.md):

```bash
ggp --ninja --ninja-output-dir-override=../../out
ninja -C out/Debug ni-randomUnitTests
xcrun xctest out/Debug/ni-randomUnitTests.xctest   # macOS
```

CI entry point is `build_scripts/ci.py` — it builds
`izo_build/ni-random.gyp:ni-randomUnitTests` with sanitizers enabled, and runs
style checks on macOS only.

**`izo_build/ni-random_sources.gypi` must be updated when adding/removing any
header or test file.** Both targets (`ni-random`, `ni-randomUnitTests`) include
the same flat `sources` list; the GYP filter `.gypi`s decide what each compiles
(`FilterUnitTestSources.gypi` compiles only `*_tests.cpp` into the test target).

## Formatting & tidy

- **There is no `.clang-format` in this repo** — clang-format's built-in **LLVM
  style** is the convention here (4-space-ish LLVM, not the tab/iZotope style used
  elsewhere in the monorepo). Format with plain `clang-format -i` /
  `git clang-format origin/HEAD`.
- Imported third-party code is reformatted to LLVM style too; only the verbatim
  MIT license block in each imported file is fenced with `// clang-format off` /
  `// clang-format on` so the required notice stays byte-exact.
- `.clang-tidy` enables a small curated check set with `WarningsAsErrors: '*'`
  (notably `cppcoreguidelines-special-member-functions`,
  `cppcoreguidelines-pro-type-member-init`, `izotope-sf-22-file-scope`,
  `bugprone-use-after-move`).

### Committing — the pre-commit hook reverts LLVM formatting

The org `format-staged` pre-commit hook re-tabs staged C++ in the index back to
the old iZotope style, silently undoing LLVM formatting (the commit succeeds but
the committed blob has tabs and the working tree diverges). **Commit with
`git -C <path> commit --no-verify`**, then verify the committed blob is LLVM
(`git show HEAD:<file>` and `clang-format --dry-run -Werror`). Use
`git -C <path>` rather than `cd && git`.

## Library architecture

The library is a **concept + conformance-suite** design. Everything orbits one
contract.

- `include/ni/random/RandomNumberEngine.hpp` — the C++20 concept modeling the
  standard `[rand.req.eng]` requirements. It checks only what is *structurally*
  checkable (member presence/signatures), via helper concepts `SeedSequence`,
  `SeedableBy<E,Sseq>`, and `StreamSerializable<E,CharT,Traits>` that pin
  explicit witnesses (`std::seed_seq`, `char` streams). Semantic requirements
  (reproducibility, `discard` equivalence, stream round-trip) are *deliberately
  not* and *cannot be* checked by the concept — they are delegated to the test
  suite. `std::minstd_rand` is the known-conforming **oracle**: a `static_assert`
  here gates it, and every conformance test must pass against it.
- `include/ni/random/<Engine>.hpp` — one header per engine (`SplitMix64`, the
  three `Xoshiro256*`, three `Xoroshiro128*`, three `Xoshiro128*`). Each ends with
  `static_assert(RandomNumberEngine<...>)`. These were imported **verbatim** from
  the MIT-licensed Xoshiro-cpp, then made concept-conformant; the MIT notice is
  reproduced in every derived file.
- `include/ni/random/detail/XoshiroDetail.hpp` — shared family helpers:
  `DefaultSeed`, `RotL`, bits→float converters, the `SeedSeqFill` family (merges
  32-bit seed-seq words into 64-bit state, remaps the all-zero fixed point), and
  the generic stream `operator<<`/`operator>>` constrained to the family via the
  `detail::XoshiroSerializable` concept (engines expose `serialize()`/
  `deserialize()` + a `state_type`; the operators wrap them and save/restore
  stream format flags). xoshiro/xoroshiro engines seed via `SplitMix64`.

### Test conformance pattern (how to add an engine)

- `test/EngineConformanceSuite.hpp` — a single type-parametrized gtest suite
  (`TYPED_TEST_SUITE_P`) holding all the generic semantic tests. **Never edit per
  engine.** It pulls every engine-specific value (seed, discard counts, golden
  value) from `EngineConformanceTraits<E>`, and probes genericity with canonical +
  adversarially-minimal witnesses (`std::seed_seq` + `MinimalSeedSeq`; `char` +
  `wchar_t`).
- `test/EngineConformanceTraits.hpp` — shared scaffolding only: the (intentionally
  undefined) primary `EngineConformanceTraits` template plus the
  `NI_RANDOM_XOSHIRO_TRAITS(Engine, Golden)` macro.
- `test/<engine>_tests.cpp` — per engine: include the engine header + suite +
  traits, then **inside `namespace ni::random::test`** invoke
  `NI_RANDOM_XOSHIRO_TRAITS(<Engine>, <golden>);` followed by
  `INSTANTIATE_TYPED_TEST_SUITE_P(<Name>, EngineConformance, ni::random::<Engine>);`.
  The traits specialization is **colocated with its instantiation**, not
  centralized. `std::minstd_rand`'s traits are hand-written in
  `minstd_rand_tests.cpp` (the macro hardcodes the `ni::random::` namespace).
- The **golden value is derived-then-pinned**: it is the `golden_n`-th (10000th)
  output of a default-constructed engine, computed once and frozen at the call
  site. The `GoldenValue` test regenerates it and fails loudly on any algorithm or
  seeding drift.

## Workflow artifacts

This repo is being built out via a plan-driven workflow with three directories:
`plans/` (self-contained task plans written to drive a fresh session),
`progress/` (execution logs tracking a plan, with decisions/notes), and
`prompts/` (kickoff prompts). When continuing multi-step work, read the matching
`progress/<task>.md` for state and decisions already made.
