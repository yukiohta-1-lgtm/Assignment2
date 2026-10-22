# Karp–Rabin vs Boyer–Moore — Programming Assignment 2

**Responsible primary author for both algorithms:** Yuki Ohta.
**AI-assisted starter implementation:** review, edit, test and disclose this work.


Two independently implemented substring-search algorithms, a correctness test
suite, a shared benchmark harness, and a results plotter.

**Important:** This is a **starting implementation**. Read it, test it, and be
able to explain the code and any changes in your assignment/video. Disclose the
AI assistance according to your course policy.

## Algorithms

- `src/karp_rabin.cpp`: rolling hash, character-by-character verification on
  hash equality. Library default base=256, modulus=1,000,000,007; the
  **benchmark selects a seeded random base** (257..65521) per input case.
- `src/boyer_moore.cpp`: **full Boyer–Moore** using both the bad-character and
  strong good-suffix rules, including overlapping matches.
- Both return **all 0-based match starting indices** (not just the first).
- Both support empty patterns (indices 0 through `text.size()`) and binary data.

Theoretical additional working memory (excluding returned match positions):
Karp–Rabin O(1) for fixed-size hash variables; Boyer–Moore O(m+256) for its
shift tables, where m is pattern length and the alphabet is byte-sized.
Hash quality strongly affects Karp–Rabin's practical collision frequency.

## 1. Compile (macOS, Windows with CMake, or Linux)

Open a terminal **inside this folder** and run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

**If CMake is not installed on your Mac:** run `./build.sh` instead
(or `sh build.sh`). This uses `clang++` and C++17, which Xcode Command Line
Tools provide; check `clang++ --version`.

For experiments, record your compiler and version (`clang++ --version` or
`g++ --version`), build flags, CPU, RAM, OS, and whether on battery/charging.
The CMake Release configuration normally enables optimization; record the exact
flags actually used (inspect `build/CMakeFiles/search_algorithms.dir/flags.make`
on Make-based systems).

## 2. Verify correctness FIRST (no benchmark results)

```sh
./build/run_tests
# or
ctest --test-dir build --output-on-failure
```

Tests compare **both** algorithms against C++ `std::string::find`, including
6000 random cases, overlapping matches, empty strings, and deliberate hash
collisions. Run these after edits to detect incorrect logic.

## 3. Freeze Section 2 predictions BEFORE timed experiments

Edit `predictions_before_experiments.md` so its predictions reflect **your own
reasoned expectations**. Save a dated copy or make a Git commit. Do NOT silently
rewrite it after seeing the measured outcomes.

## 4. Run benchmarks

Start with a small functional smoke test if needed (it DOES measure runtimes):

```sh
./build/benchmark --quick --repeats 3 --outdir results/quick
```

For the full study (run only after freezing predictions):

```sh
./build/benchmark --repeats 9 --warmups 2 --outdir results/full
```

The program prints progress and writes:
- `results_raw.csv`: one runtime per algorithm per repeated run
- `results_summary.csv`: median/min/max runtime plus diagnostic counts

Times use `std::chrono::steady_clock`, in **microseconds**. Every pair uses the
**exact same text and pattern**. Inputs are generated before timing. Both
algorithms are warmed up, and run order alternates. All found positions are
verified against a `std::string::find` reference OUTSIDE the timed region.
Diagnostics (comparisons, skips, collisions) are gathered in a separate,
**untimed** pass to avoid extra work affecting timing results. Complete
pattern preprocessing and match-result construction ARE timed for both.

Both algorithms will be affected by machine load, CPU frequency, allocator,
compiler and cache state. For more reliable results, close heavy applications,
repeat studies if desired, and retain the original raw measurements.

## Experiments and input construction

| Experiment | What changes? | Fixed values / inputs | What it probes |
| --- | --- | --- | --- |
| `text_length` | n = 10k, 50k, 100k, 500k, 1m | m=16; random alphabet=26; absent pattern | Scaling with text size |
| `pattern_length` | m = 4, 8, 16, 32, 64 | n=500k; random alphabet=26; absent pattern | Shift benefit and crossover |
| `input_type` | random vs repeated `AAAA...` | n=500k; m=16; no matches | Repetition sensitivity |
| `alphabet_size` | alphabet=4, 26, 62 | n=500k; m=16; random; no matches | Character-diversity sensitivity |
| `hash_collision` | weak mod=5 vs strong mod=1e9+7; m=16 and 64 | n=100k; same `A...A` text, `A...AF` pattern | False-positive checks |
| `match_presence` | random pattern present vs absent | n=500k; m=16; alphabet=26 | Effect of matches |

In all random **absent** cases, the generator first samples a pattern from the
same alphabet and rejects it if it appears in the text. If unsuccessful after
500 attempts, it appends a sentinel `#` absent from the text (this case gets
an explicit `_sentinel` label). Present cases choose a real text substring.

### VERY IMPORTANT: collision experiment is intentionally adversarial

- The weak modulus `5` is purposely tiny and **NOT typical practice**.
- With text consisting of `A` repeated, the pattern `AAAA...AF` has the **same
  hash** as the all-`A` window modulo 5, although it is not equal.
- Under modulo 5, every position triggers a verification that reaches the last
  character. With the default large prime this deliberate collision disappears.
- This comparison reveals a *mechanism*, not a claim that normal Karp–Rabin
  always behaves that poorly.
- Boyer–Moore's input is identical in the weak/strong pairs; its timing may
  differ slightly because of benchmarking noise, not modulus choice.

### Reading diagnostics

- `alignments`: window starts actually checked (Karp–Rabin usually checks all).
- `character_comparisons`: comparisons used for Karp–Rabin verification or
  Boyer–Moore comparisons (different work definitions: **do not interpret as
  identical total instruction costs**).
- `hash_base`: the randomized polynomial-hash base chosen from the saved seed.
- `hash_hits`: equal pattern/window hashes, including real matches.
- `false_positives`: equal hashes but unequal strings.
- `shifts`, `total_shift`: Boyer–Moore's movements; average shift =
  `total_shift / shifts` when shifts > 0.

## 5. Plot results

Install matplotlib if needed:

```sh
python3 -m pip install matplotlib
python3 scripts/plot_results.py --input results/full/results_summary.csv --output results/full/plots
```

Produces plots for text length, pattern length, input type, alphabet size,
hash collisions, and presence/absence of matches. Plot colors are automatic.

## Report roadmap

- **Section 2**: Save your theoretical expectations BEFORE timing.
- **Section 3**: Explain the exact generator settings, hardware/compiler,
  multiple timings, warmups, fairness, and collision stress design.
- **Section 4**: Use actual plots + diagnostics to explain mechanism, not merely
  which algorithm won.
- **Section 5**: Compare results to frozen predictions and investigate surprises.

## Limitations and choices to disclose

1. These implementations search all matches, while some textbooks show only
   first-match searches. Output volume can affect timings for frequent matches.
2. In the benchmark, Karp–Rabin uses a **seeded random polynomial base** for
   each input case; this is a randomized-hashing variant. The seed/base is
   recorded, and **both algorithms still return exact answers** because every
   hash hit is verified character-by-character. Only performance varies with
   hash collisions. The reference library call with its default fixed base
   remains deterministic. If strict randomized behavior is required, explain
   precisely this chosen model to the lecturer.
3. Boyer–Moore is **both** bad-character and good-suffix, not bad-character-only.
4. No Galil optimization is included; worst-case runtime depends on variant.
5. Memory usage is reasoned analytically in this starter project, **not**
   measured via an allocator or profiler.
6. Text and patterns here use single-byte characters; UTF-8 Unicode graphemes
   are outside the scope.
7. Test/reference calls, CSV writing and generation are outside timed sections.
8. Avoid presenting the draft hypotheses as measured results or claiming a
   crossover exists unless actual data supports it.
