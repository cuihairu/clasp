#ifndef CLASP_VALUE_HPP
#define CLASP_VALUE_HPP

#include <optional>
#include <string>
#include <string_view>

namespace clasp {

// pflag-like Value interface for custom flag types.
//
// Notes:
// - Clasp applies bound Values after parsing and after external sources (env/config) are merged,
//   so `set()` receives the effective final flag values in occurrence order.
// - `set()` should return an error string on failure; empty optional indicates success.
class Value {
public:
    virtual ~Value() = default;

    // A human-readable type name (e.g. "ip", "level", "duration").
    [[nodiscard]] virtual std::string type() const = 0;
    // Current value in string form (used for help/defaults when requested).
    [[nodiscard]] virtual std::string string() const = 0;
    // Parse one occurrence.
    [[nodiscard]] virtual std::optional<std::string> set(std::string_view value) = 0;
};

} // namespace clasp

#endif // CLASP_VALUE_HPP

