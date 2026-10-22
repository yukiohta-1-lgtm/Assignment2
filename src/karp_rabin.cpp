// Programming Assignment 2 — Responsible primary author: Yuki Ohta
// AI-assisted starter code: verify, adapt, and disclose its use before submission.
#include "search.hpp"

#include <stdexcept>

namespace sm {

std::vector<std::size_t> karp_rabin(
    std::string_view text, std::string_view pattern,
    HashConfig config, SearchStats* stats) {
    if (config.modulus < 2 || config.base == 0 || config.base > 65535) {
        throw std::invalid_argument("Karp-Rabin requires modulus >= 2 and 1 <= base <= 65535");
    }
    // Modulus must also be small enough that uint64_t intermediates cannot overflow.
    if (config.modulus > 1000000000000ULL) {
        throw std::invalid_argument("modulus must be <= 1e12 for safe 64-bit arithmetic");
    }
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    std::vector<std::size_t> matches;
    if (m == 0) {
        matches.reserve(n + 1);
        for (std::size_t i = 0; i <= n; ++i) matches.push_back(i);
        return matches;
    }
    if (m > n) return matches;

    const std::uint64_t base = config.base;
    const std::uint64_t modulus = config.modulus;
    std::uint64_t power = 1; // base^(m-1) mod modulus
    for (std::size_t i = 1; i < m; ++i) power = (power * base) % modulus;

    std::uint64_t pattern_hash = 0;
    std::uint64_t window_hash = 0;
    for (std::size_t i = 0; i < m; ++i) {
        pattern_hash = (pattern_hash * base + static_cast<unsigned char>(pattern[i])) % modulus;
        window_hash = (window_hash * base + static_cast<unsigned char>(text[i])) % modulus;
    }

    for (std::size_t start = 0; start <= n - m; ++start) {
        if (stats) ++stats->alignments;
        if (pattern_hash == window_hash) {
            if (stats) ++stats->hash_hits;
            std::size_t j = 0;
            for (; j < m; ++j) {
                if (stats) ++stats->character_comparisons;
                if (text[start + j] != pattern[j]) break;
            }
            if (j == m) {
                matches.push_back(start);
            } else if (stats) {
                ++stats->false_positives;
            }
        }
        if (start < n - m) {
            const std::uint64_t outgoing = static_cast<unsigned char>(text[start]);
            const std::uint64_t incoming = static_cast<unsigned char>(text[start + m]);
            const std::uint64_t removed = (outgoing * power) % modulus;
            // Avoid negative values in modular subtraction.
            window_hash = ((window_hash + modulus - removed) * base + incoming) % modulus;
        }
    }
    return matches;
}

}  // namespace sm
