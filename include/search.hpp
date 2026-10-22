#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sm {

struct SearchStats {
    std::uint64_t alignments = 0;          // Positions actually examined
    std::uint64_t character_comparisons = 0; // Actual character comparisons
    std::uint64_t hash_hits = 0;            // Karp-Rabin: equal hash values
    std::uint64_t false_positives = 0;      // Karp-Rabin: hash matched, strings did not
    std::uint64_t total_shift = 0;          // Boyer-Moore: sum of pattern shifts
    std::uint64_t shifts = 0;               // Boyer-Moore: number of shifts
};

struct HashConfig {
    std::uint64_t base = 256;
    std::uint64_t modulus = 1000000007ULL;
};

// Both return the zero-based starting indices of ALL matches, including overlaps.
// Empty pattern matches at every boundary, including text.size().
// When stats == nullptr (benchmark mode), no diagnostic counters are updated.
std::vector<std::size_t> karp_rabin(
    std::string_view text, std::string_view pattern,
    HashConfig config = {}, SearchStats* stats = nullptr);

// FULL Boyer-Moore: bad-character + good-suffix heuristics.
std::vector<std::size_t> boyer_moore(
    std::string_view text, std::string_view pattern,
    SearchStats* stats = nullptr);

}  // namespace sm
