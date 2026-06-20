# Third-Party Licenses

`ni-random` is released under the BSD 3-Clause License (see `LICENSE`). It
incorporates third-party components, each of which remains under its own
permissive license. All of these licenses are compatible with redistribution of
the combined work under BSD-3-Clause. The original copyright and license notice
for each component is preserved verbatim in the relevant source file(s); this
file aggregates those notices for attribution in binary/redistribution form,
where the per-file source headers are not shipped.

| Component | Files | Upstream | License |
|---|---|---|---|
| Xoshiro / Xoroshiro family + SplitMix64 | `include/ni/random/{SplitMix64,Xoshiro128*,Xoshiro256*,Xoroshiro128*}.hpp`, `include/ni/random/detail/XoshiroDetail.hpp` | Xoshiro-cpp (Ryo Suzuki) | MIT |
| PCG family | `include/ni/random/Pcg*.hpp`, `include/ni/random/SeedSeqFrom.hpp`, `include/ni/random/detail/Pcg{Engine,Extras,Uint128}.hpp` | pcg-cpp (Melissa O'Neill & PCG contributors) | Apache-2.0 OR MIT — distributed here under **MIT** |
| ChaCha engine | `include/ni/random/detail/ChaCha{Engine,Block}.hpp` (and wrapper `ChaCha.hpp`) | Orson Peters | zlib |
| SeedSeqFE (fixed-entropy seed sequence) | `include/ni/random/SeedSeqFE.hpp` | randutils (Melissa E. O'Neill) | MIT |
| Uniform / Normal / ApproxNormal distributions + math | `include/ni/random/{UniformInt,UniformReal,Normal,ApproxNormal}Distribution.hpp`, `include/ni/random/detail/UtlDistributionMath.hpp` | DmitriBogdanov/UTL (Dmitri Bogdanov) | MIT |
| Sobol direction numbers (data table only) | `include/ni/random/detail/SobolDirectionNumbers.hpp` | Joe-Kuo `new-joe-kuo-6.21201` (Stephen Joe & Frances Y. Kuo) | BSD-3-Clause |

> **Note on PCG (dual-licensed).** pcg-cpp is offered under "Apache-2.0 OR MIT" at
> the user's option. For this distribution we elect the **MIT** option. The MIT
> notice is reproduced below.

> **Note on ChaCha (zlib, altered).** The ChaCha source in this repository is an
> *altered version* of Orson Peters' original (the block function was factored
> into selectable scalar/SSE2/NEON policies, an ARM NEON block was added, and
> `[rand.req.eng]` conformance members were added). Per zlib clause 2 the altered
> files are plainly marked as such in their headers.

> **Note on Philox and Sobol.** The Philox engines and the Sobol generator code
> are original `ni-random` implementations written from public specifications
> (C++26 `[rand.eng.philox]`) and published algorithms (Antonov-Saleev /
> Joe-Kuo construction); no third-party source code is copied. Only the Sobol
> *direction-number data table* is reproduced from the Joe-Kuo reference file,
> whose BSD-3-Clause notice is reproduced below.

---

## Xoshiro-cpp — MIT License

```
Xoshiro-cpp
Xoshiro PRNG wrapper library for C++17 / C++20

Copyright (C) 2020 Ryo Suzuki <reputeless@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

---

## pcg-cpp — MIT License (the MIT option of Apache-2.0 OR MIT)

PCG upstream carries the SPDX identifier `(Apache-2.0 OR MIT)` and the following
notice. This distribution elects the MIT option; the standard MIT terms below
apply.

```
PCG Random Number Generation for C++

Copyright 2014-2022 Melissa O'Neill <oneill@pcg-random.org>,
                    and the PCG Project contributors.

SPDX-License-Identifier: (Apache-2.0 OR MIT)

Licensed under the Apache License, Version 2.0 (provided in
LICENSE-APACHE.txt and at http://www.apache.org/licenses/LICENSE-2.0)
or under the MIT license (provided in LICENSE-MIT.txt and at
http://opensource.org/licenses/MIT), at your option. This file may not
be copied, modified, or distributed except according to those terms.

Distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, either
express or implied.  See your chosen license for details.

For additional information about the PCG random number generation scheme,
visit http://www.pcg-random.org/.
```

MIT License (the elected option):

```
The MIT License (MIT)

Copyright 2014-2022 Melissa O'Neill <oneill@pcg-random.org>,
                    and the PCG Project contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## ChaCha — zlib License

The ChaCha source in this repository is an altered version (see note above).

```
Copyright (c) 2024 Orson Peters <orsonpeters@gmail.com>

This software is provided 'as-is', without any express or implied warranty. In no event will the
authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial
applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the
   original software. If you use this software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented as
   being the original software.

3. This notice may not be removed or altered from any source distribution.
```

---

## randutils (SeedSeqFE) — MIT License

```
Random-Number Utilities (randutil)
    Addresses common issues with C++11 random number generation.
    Makes good seeding easier, and makes using RNGs easy while retaining
    all the power.

The MIT License (MIT)

Copyright (c) 2015-2022 Melissa E. O'Neill

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## DmitriBogdanov/UTL (distributions) — MIT License

```
DmitriBogdanov/UTL — utl::random
https://github.com/DmitriBogdanov/UTL

MIT License

Copyright (c) 2023 Dmitri Bogdanov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Sobol direction numbers (Joe-Kuo) — BSD 3-Clause License

Only the direction-number data table (from `new-joe-kuo-6.21201`) is reproduced;
the surrounding Sobol code is original `ni-random` work. Source:
https://web.maths.unsw.edu.au/~fkuo/sobol/ (mirror:
https://github.com/joe-kuo/sobol_data).

```
Copyright (c) 2008, Frances Y. Kuo and Stephen Joe
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the names of the copyright holders nor the names of the
  University of New South Wales and the University of Waikato and its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```
