#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace clasp::utils {

inline std::size_t levenshteinDistance(std::string_view a, std::string_view b) {
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    if (n == 0) return m;
    if (m == 0) return n;

    std::vector<std::size_t> prev(m + 1), cur(m + 1);
    for (std::size_t j = 0; j <= m; ++j) prev[j] = j;

    for (std::size_t i = 1; i <= n; ++i) {
        cur[0] = i;
        for (std::size_t j = 1; j <= m; ++j) {
            const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        prev.swap(cur);
    }
    return prev[m];
}

inline std::vector<std::string> suggest(std::string_view input,
                                        const std::vector<std::string>& candidates,
                                        std::size_t maxResults = 3) {
    struct Scored {
        std::string value;
        std::size_t score;
    };

    std::vector<Scored> scored;
    scored.reserve(candidates.size());
    for (const auto& c : candidates) {
        if (c.empty()) continue;
        if (c.rfind(input, 0) == 0) {
            scored.push_back({c, 0});
            continue;
        }
        scored.push_back({c, levenshteinDistance(input, c)});
    }

    std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
        if (a.score != b.score) return a.score < b.score;
        return a.value < b.value;
    });

    std::vector<std::string> out;
    out.reserve(maxResults);
    for (const auto& s : scored) {
        if (out.size() >= maxResults) break;
        if (s.score <= 2) out.push_back(s.value);
    }
    return out;
}

} // namespace clasp::utils

#endif //UTILS_HPP
