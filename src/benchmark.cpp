#include "search.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {
volatile std::uint64_t output_sink = 0;  // prevent entire search from being discarded

struct InputCase {
    std::string id, experiment, type;
    std::string text, pattern;
    std::size_t alphabet_size = 0;
    std::uint64_t seed = 0;
    sm::HashConfig hash_config{};
};

struct Result {
    std::string algorithm;
    std::vector<double> microseconds;
    sm::SearchStats stats;
    std::size_t match_count = 0;
};

struct Options {
    std::size_t repetitions = 7;
    std::size_t warmups = 2;
    std::uint64_t seed = 20261012ULL;
    fs::path outdir = "results";
    bool quick = false;
};

const std::string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

std::uint64_t randomized_base(std::uint64_t seed) {
    // Seeded random polynomial base. Save both seed and base in output for
    // reproducibility. Keep base below 65536 for safe uint64_t hash arithmetic.
    std::mt19937_64 rng(seed ^ 0xD2B74407B1CE6E93ULL);
    std::uniform_int_distribution<std::uint64_t> dist(257, 65521);
    return dist(rng);
}

std::string random_string(std::size_t length, std::size_t alphabet_size,
                          std::mt19937_64& gen) {
    if (alphabet_size == 0 || alphabet_size > ALPHABET.size())
        throw std::invalid_argument("alphabet size must be between 1 and 62");
    std::uniform_int_distribution<std::size_t> dist(0, alphabet_size - 1);
    std::string s(length, 'A');
    for (char& ch : s) ch = ALPHABET[dist(gen)];
    return s;
}

InputCase random_case(std::string id, std::string experiment,
                      std::size_t n, std::size_t m, std::size_t alphabet_size,
                      std::uint64_t seed, bool with_match = false) {
    if (m == 0 || m > n) throw std::invalid_argument("bad case dimensions");
    std::mt19937_64 rng(seed);
    InputCase c;
    c.id = std::move(id);
    c.experiment = std::move(experiment);
    c.type = with_match ? "random_present" : "random_absent";
    c.alphabet_size = alphabet_size;
    c.seed = seed;
    c.hash_config.base = randomized_base(seed);
    c.text = random_string(n, alphabet_size, rng);
    if (with_match) {
        c.pattern = c.text.substr((n - m) / 2, m);
    } else {
        // Sample a genuine pattern from the same alphabet, excluding real matches.
        // Rare fallback for tiny patterns: use '#' which is not in this alphabet.
        for (int attempts = 0; attempts < 500; ++attempts) {
            c.pattern = random_string(m, alphabet_size, rng);
            if (c.text.find(c.pattern) == std::string::npos) break;
        }
        if (c.text.find(c.pattern) != std::string::npos) {
            c.pattern.back() = '#';
            c.type = "random_absent_sentinel";
        }
    }
    return c;
}

InputCase repetitive_case(std::string id, std::string experiment,
                          std::size_t n, std::size_t m, std::uint64_t seed) {
    InputCase c;
    c.id = std::move(id);
    c.experiment = std::move(experiment);
    c.type = "repetitive_absent";
    c.seed = seed;
    c.hash_config.base = randomized_base(seed);
    c.alphabet_size = 2;  // combined text/pattern distinct-character count
    c.text = std::string(n, 'A');
    c.pattern = std::string(m - 1, 'A') + 'B';
    return c;
}

InputCase collision_case(std::string id, std::size_t n, std::size_t m,
                         bool weak, std::uint64_t seed) {
    InputCase c;
    c.id = std::move(id);
    c.experiment = "hash_collision";
    c.type = weak ? "collision_weak_mod5" : "collision_strong_mod1e9+7";
    c.seed = seed;
    c.hash_config.base = randomized_base(seed);
    c.alphabet_size = 2;
    c.text = std::string(n, 'A');
    c.pattern = std::string(m - 1, 'A') + 'F';
    if (weak) c.hash_config.modulus = 5;
    // 'A'=65 and 'F'=70, so identical length-m windows have the same hash mod 5.
    // The first m-1 actual characters match, while the last character differs.
    return c;
}

std::vector<InputCase> build_cases(const Options& o) {
    std::vector<InputCase> all;
    const std::size_t common_n = o.quick ? 2000 : 500000;
    std::size_t number = 0;
    auto new_seed = [&]() { return o.seed + 1009 * (++number); };

    for (std::size_t n : (o.quick ? std::vector<std::size_t>{1000, 3000}
                                  : std::vector<std::size_t>{10000, 50000, 100000, 500000, 1000000})) {
        all.push_back(random_case("text_" + std::to_string(n), "text_length", n, 16, 26, new_seed()));
    }
    for (std::size_t m : (o.quick ? std::vector<std::size_t>{4, 16}
                                  : std::vector<std::size_t>{4, 8, 16, 32, 64})) {
        all.push_back(random_case("pattern_" + std::to_string(m), "pattern_length", common_n, m, 26, new_seed()));
    }
    all.push_back(random_case("type_random", "input_type", common_n, 16, 26, new_seed()));
    all.push_back(repetitive_case("type_repetitive", "input_type", common_n, 16, new_seed()));

    for (std::size_t alphabet : (o.quick ? std::vector<std::size_t>{4, 26}
                                        : std::vector<std::size_t>{4, 26, 62})) {
        all.push_back(random_case("alphabet_" + std::to_string(alphabet), "alphabet_size",
                                  common_n, 16, alphabet, new_seed()));
    }

    const std::size_t collision_n = o.quick ? 300 : 100000;
    for (std::size_t m : (o.quick ? std::vector<std::size_t>{16}
                                  : std::vector<std::size_t>{16, 64})) {
        const auto s = new_seed();
        all.push_back(collision_case("collision_weak_" + std::to_string(m), collision_n, m, true, s));
        all.push_back(collision_case("collision_strong_" + std::to_string(m), collision_n, m, false, s));
    }
    all.push_back(random_case("match_present", "match_presence", common_n, 16, 26, new_seed(), true));
    all.push_back(random_case("match_absent", "match_presence", common_n, 16, 26, new_seed()));
    return all;
}

std::vector<std::size_t> expected_matches(const std::string& text, const std::string& pattern) {
    std::vector<std::size_t> positions;
    std::size_t i = 0;
    while ((i = text.find(pattern, i)) != std::string::npos) {
        positions.push_back(i);
        ++i;
    }
    return positions;
}

std::vector<std::size_t> run_search(const InputCase& c,
                                    const std::string& algorithm,
                                    sm::SearchStats* stats = nullptr) {
    if (algorithm == "Karp-Rabin")
        return sm::karp_rabin(c.text, c.pattern, c.hash_config, stats);
    return sm::boyer_moore(c.text, c.pattern, stats);
}

// Run with no diagnostic instrumentation. Only algorithm work is timed:
// no input generation, standard library validation, or CSV I/O inside timer.
double timed_search(const InputCase& c, const std::string& algorithm,
                    const std::vector<std::size_t>& expected) {
    const auto start = Clock::now();
    auto matches = run_search(c, algorithm);
    const auto stop = Clock::now();
    output_sink = output_sink ^ (matches.size() + 1);
    if (matches != expected)
        throw std::runtime_error("Incorrect results in case " + c.id + " for " + algorithm);
    return std::chrono::duration<double, std::micro>(stop - start).count();
}

double median(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    const auto i = samples.size() / 2;
    return samples.size() % 2 ? samples[i] : (samples[i - 1] + samples[i]) / 2.0;
}

std::pair<Result, Result> benchmark_case(const InputCase& c, const Options& o) {
    const auto expected = expected_matches(c.text, c.pattern);
    Result rk{}, bm{};
    rk.algorithm = "Karp-Rabin";
    bm.algorithm = "Boyer-Moore";
    rk.match_count = bm.match_count = expected.size();
    if (run_search(c, rk.algorithm, &rk.stats) != expected ||
        run_search(c, bm.algorithm, &bm.stats) != expected)
        throw std::runtime_error("Correctness check failed before benchmark: " + c.id);

    for (std::size_t w = 0; w < o.warmups; ++w) {
        (void)timed_search(c, rk.algorithm, expected);
        (void)timed_search(c, bm.algorithm, expected);
    }
    for (std::size_t r = 0; r < o.repetitions; ++r) {
        // Alternate order to reduce the bias of one algorithm always running first.
        if (r % 2 == 0) {
            rk.microseconds.push_back(timed_search(c, rk.algorithm, expected));
            bm.microseconds.push_back(timed_search(c, bm.algorithm, expected));
        } else {
            bm.microseconds.push_back(timed_search(c, bm.algorithm, expected));
            rk.microseconds.push_back(timed_search(c, rk.algorithm, expected));
        }
    }
    return {std::move(rk), std::move(bm)};
}

void write_common(std::ostream& out, const InputCase& c, const Result& r) {
    out << c.id << ',' << c.experiment << ',' << c.type << ','
        << c.text.size() << ',' << c.pattern.size() << ','
        << c.alphabet_size << ',' << c.seed << ',' << c.hash_config.base << ',' << c.hash_config.modulus << ','
        << r.algorithm << ',' << r.match_count;
}

void append_result(std::ofstream& raw, std::ofstream& summary,
                   const InputCase& c, const Result& r) {
    for (std::size_t i = 0; i < r.microseconds.size(); ++i) {
        write_common(raw, c, r);
        raw << ',' << i + 1 << ',' << std::fixed << std::setprecision(3)
            << r.microseconds[i] << '\n';
    }
    write_common(summary, c, r);
    const double min = *std::min_element(r.microseconds.begin(), r.microseconds.end());
    const double max = *std::max_element(r.microseconds.begin(), r.microseconds.end());
    summary << ',' << std::fixed << std::setprecision(3) << median(r.microseconds)
            << ',' << min << ',' << max
            << ',' << r.stats.alignments
            << ',' << r.stats.character_comparisons
            << ',' << r.stats.hash_hits
            << ',' << r.stats.false_positives
            << ',' << r.stats.shifts
            << ',' << r.stats.total_shift << '\n';
}

void help() {
    std::cout << "Usage: ./build/benchmark [--outdir PATH] [--repeats N] [--warmups N] [--seed N] [--quick]\n"
                 "Produces results_raw.csv and results_summary.csv.\n"
                 "IMPORTANT: freeze your Section 2 predictions BEFORE running full experiments.\n";
}
Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { help(); std::exit(0); }
        if (arg == "--quick") { o.quick = true; continue; }
        if (i + 1 == argc) throw std::invalid_argument("Missing value after " + arg);
        const std::string val = argv[++i];
        if (arg == "--outdir") o.outdir = val;
        else if (arg == "--repeats") o.repetitions = std::stoull(val);
        else if (arg == "--warmups") o.warmups = std::stoull(val);
        else if (arg == "--seed") o.seed = std::stoull(val);
        else throw std::invalid_argument("Unknown option: " + arg);
    }
    if (o.repetitions == 0) throw std::invalid_argument("--repeats must be >= 1");
    return o;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        fs::create_directories(options.outdir);
        std::ofstream raw(options.outdir / "results_raw.csv");
        std::ofstream summary(options.outdir / "results_summary.csv");
        if (!raw || !summary) throw std::runtime_error("Could not open output files");
        constexpr auto cols = "case_id,experiment,input_type,text_length,pattern_length,alphabet_size,seed,hash_base,hash_modulus,algorithm,matches";
        raw << cols << ",repetition,time_microseconds\n";
        summary << cols << ",median_microseconds,min_microseconds,max_microseconds,alignments,character_comparisons,hash_hits,false_positives,shifts,total_shift\n";
        auto cases = build_cases(options);
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const auto& c = cases[i];
            const auto [rk, bm] = benchmark_case(c, options);
            append_result(raw, summary, c, rk);
            append_result(raw, summary, c, bm);
            std::cerr << "Validated + benchmarked " << (i + 1) << '/' << cases.size()
                      << ": " << c.id << '\n';
        }
        std::cout << "Saved " << cases.size() << " paired cases ("
                  << options.repetitions << " timed repetitions per algorithm) to "
                  << options.outdir.string() << "\n";
        // Reference so compiler cannot remove the otherwise-volatile checksum.
        if (output_sink == 0xFFFFFFFFFFFFFFFFULL) std::cerr << "checksum sentinel\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
