// Programming Assignment 2 — Responsible primary author: Yuki Ohta
// AI-assisted starter code: verify, adapt, and disclose its use before submission.
#include "search.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace sm {
namespace {
// Strong good-suffix preprocessing (standard border-position construction).
void preprocess_strong_suffix(std::string_view p,
                              std::vector<std::size_t>& shift,
                              std::vector<std::size_t>& border) {
    const std::size_t m = p.size();
    std::size_t i = m;
    std::size_t j = m + 1;
    border[i] = j;
    while (i > 0) {
        while (j <= m && p[i - 1] != p[j - 1]) {
            if (shift[j] == 0) shift[j] = j - i;
            j = border[j];
        }
        --i;
        --j;
        border[i] = j;
    }
}

void preprocess_remaining_shifts(std::vector<std::size_t>& shift,
                                 const std::vector<std::size_t>& border) {
    const std::size_t m = shift.size() - 1;
    std::size_t j = border[0];
    for (std::size_t i = 0; i <= m; ++i) {
        if (shift[i] == 0) shift[i] = j;
        if (i == j) j = border[j];
    }
}
}  // namespace

std::vector<std::size_t> boyer_moore(
    std::string_view text, std::string_view pattern, SearchStats* stats) {
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    std::vector<std::size_t> matches;
    if (m == 0) {
        matches.reserve(n + 1);
        for (std::size_t i = 0; i <= n; ++i) matches.push_back(i);
        return matches;
    }
    if (m > n) return matches;

    // Rightmost occurrence of each possible byte in the pattern.
    std::array<std::ptrdiff_t, 256> bad_character{};
    bad_character.fill(-1);
    for (std::size_t i = 0; i < m; ++i) {
        bad_character[static_cast<unsigned char>(pattern[i])] =
            static_cast<std::ptrdiff_t>(i);
    }

    std::vector<std::size_t> good_suffix_shift(m + 1, 0);
    std::vector<std::size_t> border(m + 1, 0);
    preprocess_strong_suffix(pattern, good_suffix_shift, border);
    preprocess_remaining_shifts(good_suffix_shift, border);

    std::size_t pos = 0;
    while (pos <= n - m) {
        if (stats) ++stats->alignments;
        std::ptrdiff_t j = static_cast<std::ptrdiff_t>(m) - 1;
        while (j >= 0) {
            if (stats) ++stats->character_comparisons;
            if (pattern[static_cast<std::size_t>(j)] !=
                text[pos + static_cast<std::size_t>(j)]) break;
            --j;
        }

        std::size_t step = 1;
        if (j < 0) {
            matches.push_back(pos);
            step = good_suffix_shift[0];
        } else {
            const auto mismatch = static_cast<unsigned char>(text[pos + static_cast<std::size_t>(j)]);
            const auto raw_bad_shift = j - bad_character[mismatch];
            const auto bad_shift = static_cast<std::size_t>(std::max<std::ptrdiff_t>(1, raw_bad_shift));
            const auto good_shift = good_suffix_shift[static_cast<std::size_t>(j) + 1];
            step = std::max(bad_shift, good_shift);
        }
        if (stats) {
            ++stats->shifts;
            stats->total_shift += step;
        }
        // Avoid overflow on the final shift, including extremely long strings.
        if (step > n - m - pos) break;
        pos += step;
    }
    return matches;
}

}  // namespace sm
