#include "search.hpp"

#include <cassert>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::size_t> reference(const std::string& t, const std::string& p) {
    std::vector<std::size_t> result;
    if (p.empty()) {
        for (std::size_t i = 0; i <= t.size(); ++i) result.push_back(i);
    } else {
        std::size_t at = 0;
        while ((at = t.find(p, at)) != std::string::npos) {
            result.push_back(at);
            ++at;
        }
    }
    return result;
}

void check(const std::string& text, const std::string& pattern,
           sm::HashConfig config = {}) {
    const auto expected = reference(text, pattern);
    const auto rk = sm::karp_rabin(text, pattern, config);
    const auto bm = sm::boyer_moore(text, pattern);
    if (rk != expected || bm != expected) {
        std::cerr << "FAILED: text='" << text << "' pattern='" << pattern << "'\n";
        throw std::runtime_error("search returned incorrect match positions");
    }
}
} // namespace

int main() {
    for (const auto& p : std::vector<std::string>{"", "A", "AA", "AB", "ABA", "ABAB", "AAAA", "ABCDABC", "BBC", "CAT", "F"}) {
        for (const auto& t : std::vector<std::string>{"", "A", "AAAAAA", "ABABABAB", "ABCDABCDABC", "ABCATCAT", "BBBBBC", "F", "AABAAAAAB"}) {
            check(t, p);
            check(t, p, {256, 5});
        }
    }
    std::mt19937_64 rng(20261012);
    const std::string alphabet = "ABCD";
    for (std::size_t trial = 0; trial < 6000; ++trial) {
        const std::size_t n = rng() % 110;
        const std::size_t m = rng() % 35;
        std::string t, p;
        for (std::size_t j = 0; j < n; ++j) t += alphabet[rng() % alphabet.size()];
        for (std::size_t j = 0; j < m; ++j) p += alphabet[rng() % alphabet.size()];
        check(t, p);
        check(t, p, {256, 5});
    }
    // Explicit false-positive stress input: 'A' and 'F' differ by 5.
    const std::string text(100, 'A');
    const std::string pattern = std::string(15, 'A') + 'F';
    sm::SearchStats weak_stats, strong_stats;
    auto weak = sm::karp_rabin(text, pattern, {256, 5}, &weak_stats);
    auto strong = sm::karp_rabin(text, pattern, {}, &strong_stats);
    assert(weak.empty() && strong.empty());
    assert(weak_stats.false_positives == text.size() - pattern.size() + 1);
    assert(strong_stats.false_positives == 0);
    assert(weak_stats.character_comparisons ==
           (text.size() - pattern.size() + 1) * pattern.size());
    // Support embedded zero bytes correctly.
    check(std::string("A\0BA\0B", 6), std::string("\0B", 2));
    std::cout << "PASS: fixed cases, 6000 randomized cases, collisions, and binary bytes.\n";
}
