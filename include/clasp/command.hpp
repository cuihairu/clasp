#ifndef CLASP_COMMAND_HPP
#define CLASP_COMMAND_HPP

#include <algorithm>
#include <any>
#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "flag.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "value.hpp"

namespace clasp {

class Command {
public:
    enum class ShellCompDirective : std::uint32_t {
        Default = 0,
        Error = 1,
        NoSpace = 2,
        NoFileComp = 4,
        FilterFileExt = 8,
        FilterDirs = 16,
        KeepOrder = 32,
    };

    struct CommandGroup {
        std::string id;
        std::string title;
    };

    // Mirrors cobra's general shape: Run(cmd, args).
    using Action = std::function<int(Command&, const Parser&, const std::vector<std::string>& args)>;
    // Return empty optional on success, otherwise an error message.
    using ActionE = std::function<std::optional<std::string>(Command&, const Parser&, const std::vector<std::string>& args)>;
    // Return empty optional on success, otherwise an error message.
    using ArgsValidator = std::function<std::optional<std::string>(const std::vector<std::string>& args)>;
    using Hook = std::function<void(Command&, const Parser&, const std::vector<std::string>& args)>;
    using HookE = std::function<std::optional<std::string>(Command&, const Parser&, const std::vector<std::string>& args)>;
    using CompletionFunc = std::function<std::vector<std::string>(Command&,
                                                                  const Parser&,
                                                                  const std::vector<std::string>& args,
                                                                  std::string_view toComplete)>;
    using HelpFunc = std::function<void(const Command&, std::ostream&)>;
    using UsageFunc = std::function<void(const Command&, std::ostream&)>;
    using FlagErrorFunc = std::function<std::string(const Command&, const std::string&)>;
    using NormalizeKeyFunc = std::function<std::string(std::string)>;

    explicit Command(std::string name, std::string shortDesc = {}, std::string longDesc = {})
        : name_(std::move(name)),
          short_(std::move(shortDesc)),
          long_(std::move(longDesc)) {}

    Command& setOut(std::ostream& os) {
        out_ = &os;
        return *this;
    }

    Command& setErr(std::ostream& os) {
        err_ = &os;
        return *this;
    }

    Command& disableFlagParsing(bool v = true) {
        disableFlagParsing_ = v;
        return *this;
    }

    // Cobra-like: allow unknown flags (similar to Cobra's FParseErrWhitelist.UnknownFlags).
    Command& allowUnknownFlags(bool v = true) {
        allowUnknownFlagsOverride_ = v;
        return *this;
    }

    // Cobra-like: toggle short-flag grouping (pflag style: -abc).
    Command& shortFlagGrouping(bool v = true) {
        shortFlagGroupingOverride_ = v;
        return *this;
    }

    // Cobra-like: toggle bool negation (--no-foo).
    Command& boolNegation(bool v = true) {
        boolNegationOverride_ = v;
        return *this;
    }

    // Cobra-like: normalize flag keys (e.g. treat `--foo_bar` as `--foo-bar`).
    Command& normalizeFlagKeys(NormalizeKeyFunc f) {
        normalizeFlagKeyOverride_ = std::move(f);
        return *this;
    }

    // Cobra-like: completion directive for __complete output (bitmask).
    // Default behavior matches current implementation: NoFileComp (4).
    Command& completionDirective(std::uint32_t directive) {
        completionDirectiveOverride_ = directive;
        return *this;
    }

    // Cobra-like: allow child/local flags to appear before subcommand tokens.
    Command& traverseChildren(bool v = true) {
        traverseChildren_ = v;
        return *this;
    }

    Command& bindEnv(std::string flagName, std::string envVar) {
        envBindings_[normalizeFlagName(std::move(flagName))] = std::move(envVar);
        return *this;
    }

    Command& configFile(std::string path) {
        configFilePath_ = std::move(path);
        return *this;
    }

    Command& configFileFlag(std::string flagName) {
        configFileFlag_ = normalizeFlagName(std::move(flagName));
        return *this;
    }

    Command& aliases(std::vector<std::string> a) {
        aliases_ = std::move(a);
        return *this;
    }

    // Cobra-like: free-form metadata.
    Command& annotation(std::string key, std::string value) {
        annotations_[std::move(key)] = std::move(value);
        return *this;
    }
    [[nodiscard]] const std::unordered_map<std::string, std::string>& annotations() const { return annotations_; }

    Command& addAlias(std::string a) {
        aliases_.push_back(std::move(a));
        return *this;
    }

    Command& addCommand(Command cmd) {
        auto child = std::make_unique<Command>(std::move(cmd));
        child->reparent(this);
        subcommands_.push_back(std::move(child));
        return *this;
    }

    // Cobra-like: command groups for grouped help output.
    Command& addGroup(std::string id, std::string title) {
        groups_.push_back(CommandGroup{std::move(id), std::move(title)});
        return *this;
    }

    Command& groupId(std::string id) {
        groupId_ = std::move(id);
        return *this;
    }

    Command& withFlag(std::string longName,
                      std::string shortName,
                      std::string varName,
                      std::string description,
                      FlagValue defaultValue) {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        return *this;
    }

    // pflag-like: Count flags (e.g. -vvv). Each occurrence increments; optional numeric amount can be provided as -v3 or --v=3.
    Command& withCountFlag(std::string longName,
                           std::string shortName,
                           std::string varName,
                           std::string description,
                           int defaultValue = 0) {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            defaultValue);
        flags_.back().setAnnotation("count", "true");
        return *this;
    }

    // pflag-like: bytes flag (human-friendly values like 1KB / 1MiB), stored as uint64.
    Command& withBytesFlag(std::string longName,
                           std::string shortName,
                           std::string varName,
                           std::string description,
                           std::uint64_t defaultValue = 0) {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            defaultValue);
        flags_.back().setAnnotation("bytes", "true");
        return *this;
    }

    // pflag-like: IP flag stored as string, validated/canonicalized as IPv4 or IPv6.
    Command& withIPFlag(std::string longName,
                        std::string shortName,
                        std::string varName,
                        std::string description,
                        std::string defaultValue = "") {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        flags_.back().setAnnotation("ip", "true");
        return *this;
    }

    // pflag-like: IPMask flag stored as string, validated/canonicalized as an IPv4 netmask (contiguous bits).
    Command& withIPMaskFlag(std::string longName,
                            std::string shortName,
                            std::string varName,
                            std::string description,
                            std::string defaultValue = "") {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        flags_.back().setAnnotation("ipmask", "true");
        return *this;
    }

    // pflag-like: CIDR flag stored as string, validated/canonicalized (network address + prefix).
    Command& withCIDRFlag(std::string longName,
                          std::string shortName,
                          std::string varName,
                          std::string description,
                          std::string defaultValue = "") {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        flags_.back().setAnnotation("cidr", "true");
        return *this;
    }

    // pflag-like: IPNet flag stored as string, validated/canonicalized as CIDR (network address + prefix).
    Command& withIPNetFlag(std::string longName,
                           std::string shortName,
                           std::string varName,
                           std::string description,
                           std::string defaultValue = "") {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        flags_.back().setAnnotation("ipnet", "true");
        return *this;
    }

    // pflag-like: URL flag stored as string, validated/canonicalized (best-effort).
    Command& withURLFlag(std::string longName,
                         std::string shortName,
                         std::string varName,
                         std::string description,
                         std::string defaultValue = "") {
        flags_.emplace_back(std::move(longName),
                            std::move(shortName),
                            std::move(description),
                            std::move(varName),
                            std::move(defaultValue));
        flags_.back().setAnnotation("url", "true");
        return *this;
    }

    Command& withBytesFlag(std::string longName, std::string shortName, std::string description, std::uint64_t defaultValue = 0) {
        return withBytesFlag(std::move(longName), std::move(shortName), "", std::move(description), defaultValue);
    }

    Command& withCountFlag(std::string longName, std::string shortName, std::string description, int defaultValue = 0) {
        return withCountFlag(std::move(longName), std::move(shortName), "", std::move(description), defaultValue);
    }

    Command& withIPFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withIPFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withIPMaskFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withIPMaskFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withCIDRFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withCIDRFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withIPNetFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withIPNetFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withURLFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withURLFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    // Convenience: bool flag with default false.
    Command& withFlag(std::string longName, std::string shortName, std::string description) {
        flags_.emplace_back(std::move(longName), std::move(shortName), std::move(description));
        return *this;
    }

    Command& withPersistentFlag(std::string longName,
                                std::string shortName,
                                std::string varName,
                                std::string description,
                                FlagValue defaultValue) {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        return *this;
    }

    Command& withPersistentCountFlag(std::string longName,
                                     std::string shortName,
                                     std::string varName,
                                     std::string description,
                                     int defaultValue = 0) {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      defaultValue);
        persistentFlags_.back().setAnnotation("count", "true");
        return *this;
    }

    Command& withPersistentBytesFlag(std::string longName,
                                     std::string shortName,
                                     std::string varName,
                                     std::string description,
                                     std::uint64_t defaultValue = 0) {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      defaultValue);
        persistentFlags_.back().setAnnotation("bytes", "true");
        return *this;
    }

    Command& withPersistentIPFlag(std::string longName,
                                  std::string shortName,
                                  std::string varName,
                                  std::string description,
                                  std::string defaultValue = "") {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        persistentFlags_.back().setAnnotation("ip", "true");
        return *this;
    }

    Command& withPersistentIPMaskFlag(std::string longName,
                                      std::string shortName,
                                      std::string varName,
                                      std::string description,
                                      std::string defaultValue = "") {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        persistentFlags_.back().setAnnotation("ipmask", "true");
        return *this;
    }

    Command& withPersistentCIDRFlag(std::string longName,
                                    std::string shortName,
                                    std::string varName,
                                    std::string description,
                                    std::string defaultValue = "") {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        persistentFlags_.back().setAnnotation("cidr", "true");
        return *this;
    }

    Command& withPersistentIPNetFlag(std::string longName,
                                     std::string shortName,
                                     std::string varName,
                                     std::string description,
                                     std::string defaultValue = "") {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        persistentFlags_.back().setAnnotation("ipnet", "true");
        return *this;
    }

    Command& withPersistentURLFlag(std::string longName,
                                   std::string shortName,
                                   std::string varName,
                                   std::string description,
                                   std::string defaultValue = "") {
        persistentFlags_.emplace_back(std::move(longName),
                                      std::move(shortName),
                                      std::move(description),
                                      std::move(varName),
                                      std::move(defaultValue));
        persistentFlags_.back().setAnnotation("url", "true");
        return *this;
    }

    Command& withPersistentBytesFlag(std::string longName,
                                     std::string shortName,
                                     std::string description,
                                     std::uint64_t defaultValue = 0) {
        return withPersistentBytesFlag(std::move(longName), std::move(shortName), "", std::move(description), defaultValue);
    }

    Command& withPersistentCountFlag(std::string longName,
                                     std::string shortName,
                                     std::string description,
                                     int defaultValue = 0) {
        return withPersistentCountFlag(std::move(longName), std::move(shortName), "", std::move(description), defaultValue);
    }

    Command& withPersistentIPFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withPersistentIPFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withPersistentIPMaskFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withPersistentIPMaskFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withPersistentCIDRFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withPersistentCIDRFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withPersistentIPNetFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withPersistentIPNetFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withPersistentURLFlag(std::string longName, std::string shortName, std::string description, std::string defaultValue = "") {
        return withPersistentURLFlag(std::move(longName), std::move(shortName), "", std::move(description), std::move(defaultValue));
    }

    Command& withPersistentFlag(std::string longName, std::string shortName, std::string description) {
        persistentFlags_.emplace_back(std::move(longName), std::move(shortName), std::move(description));
        return *this;
    }

    Command& markFlagRequired(const std::string& name) {
        if (auto* f = findFlagMutable(flags_, normalizeFlagName(name))) f->setRequired(true);
        return *this;
    }

    Command& markPersistentFlagRequired(const std::string& name) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setRequired(true);
        return *this;
    }

    Command& markFlagHidden(const std::string& name) {
        if (auto* f = findFlagMutable(flags_, normalizeFlagName(name))) f->setHidden(true);
        return *this;
    }

    Command& markPersistentFlagHidden(const std::string& name) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setHidden(true);
        return *this;
    }

    Command& markFlagDeprecated(const std::string& name, std::string msg) {
        if (auto* f = findFlagMutable(flags_, normalizeFlagName(name))) f->setDeprecated(std::move(msg));
        return *this;
    }

    Command& markFlagAnnotation(const std::string& name, std::string key, std::string value) {
        if (auto* f = findFlagMutable(flags_, normalizeFlagName(name))) f->setAnnotation(std::move(key), std::move(value));
        return *this;
    }

    // Cobra-like: MarkFlagFilename. Uses completion directives so shells can complete matching files.
    // `extensions` should be a list like {"yaml","yml"} (leading dots are accepted too).
    Command& markFlagFilename(const std::string& name, std::vector<std::string> extensions) {
        const auto directive = static_cast<std::uint32_t>(ShellCompDirective::FilterFileExt);
        return registerFlagCompletion(name,
                                      [extensions = std::move(extensions), directive](Command&,
                                                                                       const Parser&,
                                                                                       const std::vector<std::string>&,
                                                                                       std::string_view) {
                                          auto out = extensions;
                                          out.push_back(":" + std::to_string(directive));
                                          return out;
                                      });
    }
    Command& markFlagFilename(const std::string& name, std::initializer_list<std::string> extensions) {
        return markFlagFilename(name, std::vector<std::string>(extensions.begin(), extensions.end()));
    }

    // Cobra-like: MarkFlagDirname. Uses completion directives so shells can complete directories.
    Command& markFlagDirname(const std::string& name) {
        const auto directive = static_cast<std::uint32_t>(ShellCompDirective::FilterDirs);
        return registerFlagCompletion(name,
                                      [directive](Command&,
                                                  const Parser&,
                                                  const std::vector<std::string>&,
                                                  std::string_view) { return std::vector<std::string>{":" + std::to_string(directive)}; });
    }

    // Cobra/pflag: NoOptDefVal (allow omitting value for non-bool flags).
    Command& markFlagNoOptDefaultValue(const std::string& name, std::string value) {
        if (auto* f = findFlagMutable(flags_, normalizeFlagName(name))) f->setNoOptDefaultValue(std::move(value));
        return *this;
    }

    Command& markPersistentFlagDeprecated(const std::string& name, std::string msg) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setDeprecated(std::move(msg));
        return *this;
    }

    Command& markPersistentFlagAnnotation(const std::string& name, std::string key, std::string value) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setAnnotation(std::move(key), std::move(value));
        return *this;
    }

    Command& markPersistentFlagFilename(const std::string& name, std::vector<std::string> extensions) {
        return markFlagFilename(name, std::move(extensions));
    }
    Command& markPersistentFlagFilename(const std::string& name, std::initializer_list<std::string> extensions) {
        return markFlagFilename(name, extensions);
    }
    Command& markPersistentFlagDirname(const std::string& name) { return markFlagDirname(name); }

    Command& markPersistentFlagNoOptDefaultValue(const std::string& name, std::string value) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setNoOptDefaultValue(std::move(value));
        return *this;
    }

    // Cobra-like flag group constraints.
    // Mutually exclusive: 0 or 1 of the group may be set.
    Command& markFlagsMutuallyExclusive(std::vector<std::string> names) {
        std::vector<std::string> group;
        group.reserve(names.size());
        for (auto& n : names) group.push_back(normalizeFlagName(std::move(n)));
        if (!group.empty()) mutuallyExclusiveFlagGroups_.push_back(std::move(group));
        return *this;
    }
    Command& markFlagsMutuallyExclusive(std::initializer_list<std::string> names) {
        return markFlagsMutuallyExclusive(std::vector<std::string>(names.begin(), names.end()));
    }

    // One required: at least 1 of the group must be set.
    Command& markFlagsOneRequired(std::vector<std::string> names) {
        std::vector<std::string> group;
        group.reserve(names.size());
        for (auto& n : names) group.push_back(normalizeFlagName(std::move(n)));
        if (!group.empty()) oneRequiredFlagGroups_.push_back(std::move(group));
        return *this;
    }
    Command& markFlagsOneRequired(std::initializer_list<std::string> names) {
        return markFlagsOneRequired(std::vector<std::string>(names.begin(), names.end()));
    }

    // Required together: either none or all flags in group must be set.
    Command& markFlagsRequiredTogether(std::vector<std::string> names) {
        std::vector<std::string> group;
        group.reserve(names.size());
        for (auto& n : names) group.push_back(normalizeFlagName(std::move(n)));
        if (!group.empty()) requiredTogetherFlagGroups_.push_back(std::move(group));
        return *this;
    }
    Command& markFlagsRequiredTogether(std::initializer_list<std::string> names) {
        return markFlagsRequiredTogether(std::vector<std::string>(names.begin(), names.end()));
    }

    // Cobra-like: completion candidates for positional args.
    Command& validArgs(std::vector<std::string> values) {
        validArgs_ = std::move(values);
        return *this;
    }

    // Cobra-like: dynamic completion for positional args.
    Command& validArgsFunction(CompletionFunc f) {
        validArgsFunction_ = std::move(f);
        return *this;
    }

    // Cobra-like: flag value completion (for non-bool flags).
    Command& registerFlagCompletion(std::string flagName, CompletionFunc f) {
        flagCompletionFuncs_[normalizeFlagName(std::move(flagName))] = std::move(f);
        return *this;
    }

    // pflag-like: bind a custom Value interface to a flag name (long or short, will be normalized).
    // The bound Value is applied after parsing (and after env/config merge).
    Command& bindFlagValue(std::string flagName, Value& v) {
        flagValueBindings_[normalizeFlagName(std::move(flagName))] = &v;
        return *this;
    }

    // Convenience: declare a string-valued flag and bind it to a custom Value.
    // If `defaultValue` is empty, the Value's current `string()` is used for help/defaults.
    Command& withValueFlag(std::string longName,
                           std::string shortName,
                           std::string varName,
                           std::string description,
                           Value& v,
                           std::string defaultValue = {}) {
        const auto def = defaultValue.empty() ? v.string() : defaultValue;
        withFlag(longName, std::move(shortName), std::move(varName), std::move(description), def);
        bindFlagValue(std::move(longName), v);
        return *this;
    }
    Command& withValueFlag(std::string longName, std::string shortName, std::string description, Value& v, std::string defaultValue = {}) {
        return withValueFlag(std::move(longName), std::move(shortName), "", std::move(description), v, std::move(defaultValue));
    }

    Command& action(Action action) {
        action_ = std::move(action);
        return *this;
    }

    Command& actionE(ActionE action) {
        actionE_ = std::move(action);
        return *this;
    }

    Command& args(ArgsValidator validator) {
        args_ = std::move(validator);
        return *this;
    }

    Command& preRun(Hook h) {
        preRun_ = std::move(h);
        return *this;
    }
    Command& preRunE(HookE h) {
        preRunE_ = std::move(h);
        return *this;
    }
    Command& postRun(Hook h) {
        postRun_ = std::move(h);
        return *this;
    }
    Command& postRunE(HookE h) {
        postRunE_ = std::move(h);
        return *this;
    }
    Command& persistentPreRun(Hook h) {
        persistentPreRun_ = std::move(h);
        return *this;
    }
    Command& persistentPreRunE(HookE h) {
        persistentPreRunE_ = std::move(h);
        return *this;
    }
    Command& persistentPostRun(Hook h) {
        persistentPostRun_ = std::move(h);
        return *this;
    }
    Command& persistentPostRunE(HookE h) {
        persistentPostRunE_ = std::move(h);
        return *this;
    }

    Command& version(std::string v) {
        version_ = std::move(v);
        return *this;
    }

    // Cobra-like: custom version template.
    // Supports: {{.Version}}, {{.CommandPath}}, {{.Name}}.
    Command& setVersionTemplate(std::string tpl) {
        versionTemplateOverride_ = std::move(tpl);
        return *this;
    }

    Command& hidden(bool v = true) {
        hidden_ = v;
        return *this;
    }

    Command& deprecated(std::string msg) {
        deprecated_ = std::move(msg);
        return *this;
    }

    // Cobra-like: show examples in help output.
    Command& example(std::string ex) {
        example_ = std::move(ex);
        return *this;
    }
    Command& examples(std::string ex) { return example(std::move(ex)); }

    Command& silenceUsage(bool v = true) {
        silenceUsage_ = v;
        return *this;
    }

    Command& silenceErrors(bool v = true) {
        silenceErrors_ = v;
        return *this;
    }

    Command& suggestions(bool v = true) {
        suggestions_ = v;
        suggestionsOverride_ = v;
        return *this;
    }

    // Cobra-like: controls the max Levenshtein distance for suggestions (default 2).
    Command& suggestionsMinimumDistance(std::size_t d) {
        suggestionsMinimumDistance_ = d;
        suggestionsMinimumDistanceOverride_ = d;
        return *this;
    }

    // Cobra-like: sorting behavior (Cobra sorts commands/flags by default).
    Command& disableSortCommands(bool v = true) {
        disableSortCommandsOverride_ = v;
        return *this;
    }

    Command& disableSortFlags(bool v = true) {
        disableSortFlagsOverride_ = v;
        return *this;
    }

    // Cobra-like: DisableFlagsInUseLine (hide "[flags]" in Usage line).
    Command& disableFlagsInUseLine(bool v = true) {
        disableFlagsInUseLineOverride_ = v;
        return *this;
    }

    // Cobra-like: custom help/usage templates & funcs.
    // Templates support a small placeholder set: {{.UsageLine}}, {{.ShortSection}}, {{.ExamplesSection}},
    // {{.CommandsSection}}, {{.FlagsSection}}, {{.GlobalFlagsSection}}, {{.CommandPath}}.
    Command& setHelpTemplate(std::string tpl) {
        helpTemplateOverride_ = std::move(tpl);
        return *this;
    }

    Command& setUsageTemplate(std::string tpl) {
        usageTemplateOverride_ = std::move(tpl);
        return *this;
    }

    Command& setHelpFunc(HelpFunc f) {
        helpFuncOverride_ = std::move(f);
        return *this;
    }

    Command& setUsageFunc(UsageFunc f) {
        usageFuncOverride_ = std::move(f);
        return *this;
    }

    // Cobra-like: SetFlagErrorFunc (customize flag parse errors).
    Command& setFlagErrorFunc(FlagErrorFunc f) {
        flagErrorFuncOverride_ = std::move(f);
        return *this;
    }

    struct HelpConfig {
        bool addHelpCommand{true};
        std::string helpCommandName{"help"};
    };

    // Cobra-like: SetHelpCommand-style customization (rename/disable the built-in root help command).
    Command& enableHelp() { return enableHelp(HelpConfig{}); }

    Command& enableHelp(HelpConfig cfg) {
        addHelpCommandOverride_ = cfg.addHelpCommand;
        helpCommandNameOverride_ = std::move(cfg.helpCommandName);
        return *this;
    }

    Command& disableHelpCommand(bool v = true) {
        addHelpCommandOverride_ = !v;
        return *this;
    }

    // Cobra-like: programmatic execution helpers.
    // `setArgs` mirrors Cobra's SetArgs: args exclude the program name.
    Command& setArgs(std::vector<std::string> args) {
        argsOverride_ = std::move(args);
        return *this;
    }

    // Cobra-like: ExecuteContext-style propagation. Context is inherited from parents.
    Command& setContext(std::any ctx) {
        contextOverride_ = std::move(ctx);
        return *this;
    }

    int runWithContext(std::any ctx, int argc, char** argv) {
        auto old = std::move(contextOverride_);
        contextOverride_ = std::move(ctx);
        const int rc = run(argc, argv);
        contextOverride_ = std::move(old);
        return rc;
    }

    int executeWithContext(std::any ctx) {
        auto old = std::move(contextOverride_);
        contextOverride_ = std::move(ctx);
        const int rc = execute();
        contextOverride_ = std::move(old);
        return rc;
    }

    bool hasContext() const { return resolvedContext() != nullptr; }

    template <typename T>
    const T* contextAs() const {
        const auto* a = resolvedContext();
        if (!a) return nullptr;
        return std::any_cast<T>(a);
    }

    template <typename T>
    T* contextAs() {
        auto* a = resolvedContextMutable();
        if (!a) return nullptr;
        return std::any_cast<T>(a);
    }

    // Execute with previously set args; if none set, executes with no args.
    int execute() {
        std::vector<std::string> argvVec;
        argvVec.reserve(1 + (argsOverride_.has_value() ? argsOverride_->size() : 0));
        argvVec.push_back(name_);
        if (argsOverride_.has_value()) {
            argvVec.insert(argvVec.end(), argsOverride_->begin(), argsOverride_->end());
        }

        std::vector<char*> argvPtrs;
        argvPtrs.reserve(argvVec.size());
        for (auto& s : argvVec) argvPtrs.push_back(s.data());
        return run(static_cast<int>(argvVec.size()), argvPtrs.data());
    }

    int execute(int argc, char** argv) { return run(argc, argv); }

    int run(int argc, char** argv) {
        auto resolution = resolveForExecution(argc, argv);
        if (resolution.helpRequested) {
            return execHelp(*resolution.helpBase, resolution.helpPath);
        }
        if (resolution.versionRequested) {
            const auto v = resolution.cmd->buildVersionText();
            if (v.empty()) return 0;
            resolution.cmd->out() << v << "\n";
            return 0;
        }

        auto argvVec = std::move(resolution.argvForCmd);
        std::vector<char*> argvPtrs;
        argvPtrs.reserve(argvVec.size());
        for (auto& s : argvVec) argvPtrs.push_back(s.data());

        const auto effectiveFlags = resolution.cmd->effectiveFlags();
        Parser::Options parseOpts;
        parseOpts.disableFlagParsing = resolution.cmd->disableFlagParsing_;
        parseOpts.allowUnknownFlags = resolution.cmd->resolvedAllowUnknownFlags();
        parseOpts.shortFlagGrouping = resolution.cmd->resolvedShortFlagGrouping();
        parseOpts.boolNegation = resolution.cmd->resolvedBoolNegation();
        parseOpts.suggestFlags = resolution.cmd->resolvedSuggestions();
        parseOpts.suggestionsMinimumDistance = resolution.cmd->resolvedSuggestionsMinimumDistance();
        if (const auto* norm = resolution.cmd->resolvedNormalizeFlagKey()) parseOpts.normalizeKey = *norm;
        Parser parser(static_cast<int>(argvVec.size()), argvPtrs.data(), effectiveFlags, parseOpts);
        if (!parser.ok()) {
            auto msg = parser.error();
            if (const auto* fe = resolution.cmd->resolvedFlagErrorFunc()) msg = (*fe)(*resolution.cmd, msg);
            return resolution.cmd->fail(std::move(msg), /*showUsage=*/true);
        }

        if (parser.hasFlag("--help") || parser.hasFlag("-h")) {
            resolution.cmd->printHelp();
            return 0;
        }

        const auto& positionals = parser.positionals();

        // `help` and `version` subcommands are treated as root-only, cobra-like.
        if (resolution.cmd == this && resolvedAddHelpCommand() && !positionals.empty() &&
            positionals.front() == resolvedHelpCommandName()) {
            std::vector<std::string> path(positionals.begin() + 1, positionals.end());
            return execHelp(*this, path);
        }
        if (resolution.cmd == this && !positionals.empty() && positionals.front() == "version") {
            const auto v = buildVersionText();
            if (!v.empty()) out() << v << "\n";
            return 0;
        }

        // Deprecation warnings (cobra-like: warn but continue).
        if (!resolution.cmd->deprecated_.empty()) {
            resolution.cmd->err() << "Command \"" << resolution.cmd->commandPath()
                                  << "\" is deprecated: " << resolution.cmd->deprecated_ << "\n";
        }
        resolution.cmd->warnDeprecatedFlags(parser);

        if (auto err = resolution.cmd->applyExternalSources(parser)) {
            return resolution.cmd->fail(*err, /*showUsage=*/true);
        }

        if (auto err = resolution.cmd->applyBoundFlagValues(parser)) {
            auto msg = *err;
            if (const auto* fe = resolution.cmd->resolvedFlagErrorFunc()) msg = (*fe)(*resolution.cmd, msg);
            return resolution.cmd->fail(std::move(msg), /*showUsage=*/true);
        }

        // Version flag is global.
        if (parser.hasFlag("--version")) {
            const auto v = resolution.cmd->buildVersionText();
            if (!v.empty()) resolution.cmd->out() << v << "\n";
            return 0;
        }

        if (resolution.cmd->args_) {
            if (auto err = resolution.cmd->args_(positionals)) {
                return resolution.cmd->fail(*err, /*showUsage=*/true);
            }
        }

        if (auto err = resolution.cmd->checkRequiredFlags(parser)) {
            auto msg = *err;
            if (const auto* fe = resolution.cmd->resolvedFlagErrorFunc()) msg = (*fe)(*resolution.cmd, msg);
            return resolution.cmd->fail(std::move(msg), /*showUsage=*/true);
        }

        if (auto err = resolution.cmd->checkFlagGroups(parser)) {
            auto msg = *err;
            if (const auto* fe = resolution.cmd->resolvedFlagErrorFunc()) msg = (*fe)(*resolution.cmd, msg);
            return resolution.cmd->fail(std::move(msg), /*showUsage=*/true);
        }

        if (!resolution.cmd->runnable()) {
            if (positionals.empty()) {
                resolution.cmd->printHelp();
                return 0;
            }
            return resolution.cmd->failUnknownCommand(positionals.front());
        }

        const auto outcome = resolution.cmd->runHooksAndAction(parser, positionals);
        if (std::holds_alternative<std::string>(outcome)) {
            return resolution.cmd->fail(std::get<std::string>(outcome), /*showUsage=*/true);
        }
        return std::get<int>(outcome);
    }

    void printHelp() const {
        if (const auto* hf = resolvedHelpFunc()) {
            (*hf)(*this, out());
            return;
        }

        if (const auto* tpl = resolvedHelpTemplate()) {
            out() << renderTemplate(
                *tpl,
                {
                    {"CommandPath", commandPath()},
                    {"UsageLine", buildUsageLine()},
                    {"ShortSection", buildShortSection()},
                    {"ExamplesSection", buildExamplesSection()},
                    {"CommandsSection", buildCommandsSection()},
                    {"FlagsSection", buildFlagsSection()},
                    {"GlobalFlagsSection", buildGlobalFlagsSection()},
                });
            return;
        }

        printUsageTo(out());

        const auto cmdShort = short_.empty() ? long_ : short_;
        if (!cmdShort.empty()) out() << "\n" << cmdShort << "\n";

        if (!example_.empty()) {
            out() << "\nExamples:\n";
            std::istringstream iss(example_);
            std::string line;
            while (std::getline(iss, line)) out() << "  " << line << "\n";
        }

        const auto visibleSubcommands = listVisibleSubcommands();
        const bool showCommands = !visibleSubcommands.empty() || (isRoot() && suggestions_);
        if (showCommands) {
            out() << "\nCommands:\n";
            if (isRoot() && suggestions_ && resolvedAddHelpCommand()) {
                out() << "  " << resolvedHelpCommandName() << " - Help about any command\n";
            }
            if (isRoot() && suggestions_ && !resolvedVersion().empty()) out() << "  version - Print the version number\n";

            if (groups_.empty()) {
                for (const auto* sub : visibleSubcommands) {
                    out() << "  " << sub->name_ << " - " << sub->short_ << "\n";
                }
            } else {
                std::unordered_map<std::string, std::vector<const Command*>> byGroup;
                std::vector<const Command*> ungrouped;
                ungrouped.reserve(visibleSubcommands.size());

                for (const auto* sub : visibleSubcommands) {
                    if (sub->groupId_.empty()) {
                        ungrouped.push_back(sub);
                        continue;
                    }
                    bool known = false;
                    for (const auto& g : groups_) {
                        if (g.id == sub->groupId_) {
                            known = true;
                            break;
                        }
                    }
                    if (!known) {
                        ungrouped.push_back(sub);
                        continue;
                    }
                    byGroup[sub->groupId_].push_back(sub);
                }

                for (const auto* sub : ungrouped) {
                    out() << "  " << sub->name_ << " - " << sub->short_ << "\n";
                }

                for (const auto& g : groups_) {
                    const auto it = byGroup.find(g.id);
                    if (it == byGroup.end() || it->second.empty()) continue;
                    out() << "\n" << g.title << ":\n";
                    for (const auto* sub : it->second) {
                        out() << "  " << sub->name_ << " - " << sub->short_ << "\n";
                    }
                }
            }
        }

        const auto [localFlags, globalFlags] = flagsForHelp();
        if (!localFlags.empty()) {
            out() << "\nFlags:\n";
            for (const auto* f : localFlags) out() << "  " << formatFlagForHelp(*f) << "\n";
        }
        if (!globalFlags.empty()) {
            out() << "\nGlobal Flags:\n";
            for (const auto* f : globalFlags) out() << "  " << formatFlagForHelp(*f) << "\n";
        }
    }

    const std::string& name() const { return name_; }
    std::string commandPath() const {
        std::vector<const Command*> chain;
        for (auto* c = this; c; c = c->parent_) chain.push_back(c);
        std::string out;
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            if (!out.empty()) out += " ";
            out += (*it)->name_;
        }
        return out;
    }

    void printMarkdown(std::ostream& os, bool recursive = true) const {
        os << "# " << commandPath() << "\n\n";
        if (!short_.empty()) os << short_ << "\n\n";
        if (!long_.empty() && long_ != short_) os << long_ << "\n\n";

        if (!example_.empty()) {
            os << "## Examples\n\n```text\n" << example_ << "\n```\n";
        }

        os << "## Usage\n\n```text\n";
        printUsageTo(os);
        os << "```\n";

        const auto visibleSubcommands = listVisibleSubcommands();
        if (!visibleSubcommands.empty()) {
            os << "\n## Commands\n";
            for (const auto* sub : visibleSubcommands) {
                os << "- `" << sub->name_ << "`: " << sub->short_ << "\n";
            }
        }

        const auto [localFlags, globalFlags] = flagsForHelp();
        if (!localFlags.empty()) {
            os << "\n## Flags\n";
            for (const auto* f : localFlags) os << "- `" << f->longName() << "`: " << f->description() << "\n";
        }
        if (!globalFlags.empty()) {
            os << "\n## Global Flags\n";
            for (const auto* f : globalFlags) os << "- `" << f->longName() << "`: " << f->description() << "\n";
        }

        if (recursive) {
            for (const auto* sub : visibleSubcommands) {
                os << "\n---\n\n";
                sub->printMarkdown(os, /*recursive=*/true);
            }
        }
    }

    void printManpage(std::ostream& os) const {
        const std::string title = commandPath();
        os << ".TH \"" << title << "\" \"1\"\n";
        os << ".SH NAME\n";
        os << title << "\n";
        os << ".SH SYNOPSIS\n";
        os << ".nf\n";
        printUsageTo(os);
        os << ".fi\n";

        const auto cmdShort = short_.empty() ? long_ : short_;
        if (!cmdShort.empty()) {
            os << ".SH DESCRIPTION\n";
            os << cmdShort << "\n";
        }

        if (!example_.empty()) {
            os << ".SH EXAMPLES\n";
            os << ".nf\n";
            os << example_ << "\n";
            os << ".fi\n";
        }

        const auto visibleSubcommands = listVisibleSubcommands();
        if (!visibleSubcommands.empty()) {
            os << ".SH COMMANDS\n";
            for (const auto* sub : visibleSubcommands) {
                os << ".TP\n";
                os << sub->name_ << "\n";
                os << sub->short_ << "\n";
            }
        }
    }

    void printCompletionBash(std::ostream& os) const { printCompletionBashInternal(os, /*forZsh=*/false); }

    void printCompletionZsh(std::ostream& os) const { printCompletionBashInternal(os, /*forZsh=*/true); }

    void printCompletionFish(std::ostream& os) const;

    void printCompletionPowerShell(std::ostream& os) const;

    struct CompletionConfig {
        bool addCompletionCommand{true};
        std::string completionCommandName{"completion"};
        bool addCompleteCommands{true};
        std::string completeCommandName{"__complete"};
        std::string completeNoDescCommandName{"__completeNoDesc"};
    };

    Command& enableCompletion();
    Command& enableCompletion(CompletionConfig cfg);

    std::ostream& outOrStdout() const { return out(); }
    std::ostream& errOrStderr() const { return err(); }

private:
    struct Resolution {
        Command* cmd{nullptr};
        std::vector<std::string> argvForCmd;
        bool helpRequested{false};
        bool versionRequested{false};
        Command* helpBase{nullptr};
        std::vector<std::string> helpPath;
    };

    Command* findSubcommand(const std::string& name) {
        for (auto& c : subcommands_) {
            if (c->name_ == name) return c.get();
            for (const auto& a : c->aliases_) {
                if (a == name) return c.get();
            }
        }
        return nullptr;
    }

    const Command* findSubcommand(const std::string& name) const {
        for (const auto& c : subcommands_) {
            if (c->name_ == name) return c.get();
            for (const auto& a : c->aliases_) {
                if (a == name) return c.get();
            }
        }
        return nullptr;
    }

    static bool isFlagToken(const std::string& s) {
        return s.size() >= 2 && s[0] == '-' && s != "-";
    }

    static std::string normalizeFlagName(std::string n) {
        if (n.rfind("--", 0) == 0 || n.rfind("-", 0) == 0) return n;
        return std::string("--") + n;
    }

    static Flag* findFlagMutable(std::vector<Flag>& flags, const std::string& longName) {
        for (auto& f : flags) {
            if (f.longName() == longName) return &f;
        }
        return nullptr;
    }

    std::vector<Flag> effectiveFlags() const {
        std::vector<Flag> out;
        const auto inherited = inheritedPersistentFlags();
        out.reserve(inherited.size() + flags_.size() + persistentFlags_.size());
        for (const auto* f : inherited) out.push_back(*f);
        for (const auto& f : persistentFlags_) out.push_back(f);
        for (const auto& f : flags_) out.push_back(f);
        return out;
    }

    std::vector<const Flag*> inheritedPersistentFlags() const {
        std::vector<const Flag*> out;
        std::vector<const Command*> chain;
        for (auto* c = parent_; c; c = c->parent_) chain.push_back(c);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            for (const auto& f : (*it)->persistentFlags_) out.push_back(&f);
        }
        return out;
    }

    std::string resolvedVersion() const {
        for (auto* c = this; c; c = c->parent_) {
            if (!c->version_.empty()) return c->version_;
        }
        return {};
    }

    const std::string* resolvedVersionTemplate() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->versionTemplateOverride_.has_value()) return &(*c->versionTemplateOverride_);
        }
        return nullptr;
    }

    bool resolvedSuggestions() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->suggestionsOverride_.has_value()) return *c->suggestionsOverride_;
        }
        return true;
    }

    bool resolvedDisableSortCommands() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->disableSortCommandsOverride_.has_value()) return *c->disableSortCommandsOverride_;
        }
        return false;
    }

    bool resolvedDisableSortFlags() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->disableSortFlagsOverride_.has_value()) return *c->disableSortFlagsOverride_;
        }
        return false;
    }

    bool resolvedDisableFlagsInUseLine() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->disableFlagsInUseLineOverride_.has_value()) return *c->disableFlagsInUseLineOverride_;
        }
        return false;
    }

    bool resolvedAddHelpCommand() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->addHelpCommandOverride_.has_value()) return *c->addHelpCommandOverride_;
        }
        return true;
    }

    std::string resolvedHelpCommandName() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->helpCommandNameOverride_.has_value()) return *c->helpCommandNameOverride_;
        }
        return "help";
    }

    const HelpFunc* resolvedHelpFunc() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->helpFuncOverride_.has_value()) return &(*c->helpFuncOverride_);
        }
        return nullptr;
    }

    const UsageFunc* resolvedUsageFunc() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->usageFuncOverride_.has_value()) return &(*c->usageFuncOverride_);
        }
        return nullptr;
    }

    const FlagErrorFunc* resolvedFlagErrorFunc() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->flagErrorFuncOverride_.has_value()) return &(*c->flagErrorFuncOverride_);
        }
        return nullptr;
    }

    const std::string* resolvedHelpTemplate() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->helpTemplateOverride_.has_value()) return &(*c->helpTemplateOverride_);
        }
        return nullptr;
    }

    const std::string* resolvedUsageTemplate() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->usageTemplateOverride_.has_value()) return &(*c->usageTemplateOverride_);
        }
        return nullptr;
    }

    std::string buildVersionText() const {
        const auto v = resolvedVersion();
        if (v.empty()) return {};
        const auto* tpl = resolvedVersionTemplate();
        if (!tpl) return v;
        std::unordered_map<std::string, std::string> vars;
        vars.emplace("Version", v);
        vars.emplace("CommandPath", commandPath());
        vars.emplace("Name", name_);
        return renderTemplate(*tpl, vars);
    }

    bool resolvedAllowUnknownFlags() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->allowUnknownFlagsOverride_.has_value()) return *c->allowUnknownFlagsOverride_;
        }
        return false;
    }

    bool resolvedShortFlagGrouping() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->shortFlagGroupingOverride_.has_value()) return *c->shortFlagGroupingOverride_;
        }
        return true;
    }

    bool resolvedBoolNegation() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->boolNegationOverride_.has_value()) return *c->boolNegationOverride_;
        }
        return true;
    }

    const NormalizeKeyFunc* resolvedNormalizeFlagKey() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->normalizeFlagKeyOverride_.has_value()) return &(*c->normalizeFlagKeyOverride_);
        }
        return nullptr;
    }

    std::uint32_t resolvedCompletionDirective() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->completionDirectiveOverride_.has_value()) return *c->completionDirectiveOverride_;
        }
        // Keep prior behavior: always print ":4" (NoFileComp) unless overridden.
        return static_cast<std::uint32_t>(ShellCompDirective::NoFileComp);
    }

    CompletionConfig resolvedCompletionConfig() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->completionConfigOverride_.has_value()) return *c->completionConfigOverride_;
        }
        return CompletionConfig{};
    }

    const std::any* resolvedContext() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->contextOverride_.has_value()) return &(*c->contextOverride_);
        }
        return nullptr;
    }

    std::any* resolvedContextMutable() {
        for (auto* c = this; c; c = c->parent_) {
            if (c->contextOverride_.has_value()) return &(*c->contextOverride_);
        }
        return nullptr;
    }

    std::string normalizeFlagKeyForLookup(std::string key) const {
        if (const auto* f = resolvedNormalizeFlagKey()) key = (*f)(std::move(key));
        return key;
    }

    std::size_t resolvedSuggestionsMinimumDistance() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->suggestionsMinimumDistanceOverride_.has_value()) return *c->suggestionsMinimumDistanceOverride_;
        }
        return 2;
    }

    bool runnable() const { return static_cast<bool>(action_) || static_cast<bool>(actionE_); }

    void printUsageTo(std::ostream& os) const {
        if (const auto* uf = resolvedUsageFunc()) {
            (*uf)(*this, os);
            return;
        }
        if (const auto* tpl = resolvedUsageTemplate()) {
            os << renderTemplate(*tpl,
                                 {
                                     {"CommandPath", commandPath()},
                                     {"UsageLine", buildUsageLine()},
                                 });
            return;
        }
        os << buildUsageLine();
    }

    bool isRoot() const { return parent_ == nullptr; }

    std::ostream& out() const {
        if (out_) return *out_;
        if (parent_) return parent_->out();
        return std::cout;
    }

    std::ostream& err() const {
        if (err_) return *err_;
        if (parent_) return parent_->err();
        return std::cerr;
    }

    std::vector<const Command*> listVisibleSubcommands() const {
        std::vector<const Command*> out;
        for (const auto& c : subcommands_) {
            if (c->hidden_) continue;
            out.push_back(c.get());
        }
        if (!resolvedDisableSortCommands()) {
            std::sort(out.begin(), out.end(), [](const Command* a, const Command* b) { return a->name_ < b->name_; });
        }
        return out;
    }

    std::string buildUsageLine() const {
        std::ostringstream oss;
        oss << "Usage: " << commandPath();
        if (!listVisibleSubcommands().empty() || (isRoot() && suggestions_)) {
            oss << " [command]";
        }
        if (!resolvedDisableFlagsInUseLine()) {
            oss << " [flags]";
        }
        oss << "\n";
        return oss.str();
    }

    std::string buildShortSection() const {
        const auto cmdShort = short_.empty() ? long_ : short_;
        if (cmdShort.empty()) return {};
        return "\n" + cmdShort + "\n";
    }

    std::string buildExamplesSection() const {
        if (example_.empty()) return {};
        std::ostringstream oss;
        oss << "\nExamples:\n";
        std::istringstream iss(example_);
        std::string line;
        while (std::getline(iss, line)) oss << "  " << line << "\n";
        return oss.str();
    }

    std::string buildCommandsSection() const {
        const auto visibleSubcommands = listVisibleSubcommands();
        const bool showCommands = !visibleSubcommands.empty() || (isRoot() && suggestions_);
        if (!showCommands) return {};

        std::ostringstream oss;
        oss << "\nCommands:\n";
        if (isRoot() && suggestions_ && resolvedAddHelpCommand()) {
            oss << "  " << resolvedHelpCommandName() << " - Help about any command\n";
        }
        if (isRoot() && suggestions_ && !resolvedVersion().empty()) oss << "  version - Print the version number\n";

        if (groups_.empty()) {
            for (const auto* sub : visibleSubcommands) {
                oss << "  " << sub->name_ << " - " << sub->short_ << "\n";
            }
            return oss.str();
        }

        std::unordered_map<std::string, std::vector<const Command*>> byGroup;
        std::vector<const Command*> ungrouped;
        ungrouped.reserve(visibleSubcommands.size());

        for (const auto* sub : visibleSubcommands) {
            if (sub->groupId_.empty()) {
                ungrouped.push_back(sub);
                continue;
            }
            bool known = false;
            for (const auto& g : groups_) {
                if (g.id == sub->groupId_) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                ungrouped.push_back(sub);
                continue;
            }
            byGroup[sub->groupId_].push_back(sub);
        }

        for (const auto* sub : ungrouped) {
            oss << "  " << sub->name_ << " - " << sub->short_ << "\n";
        }

        for (const auto& g : groups_) {
            const auto it = byGroup.find(g.id);
            if (it == byGroup.end() || it->second.empty()) continue;
            oss << "\n" << g.title << ":\n";
            for (const auto* sub : it->second) {
                oss << "  " << sub->name_ << " - " << sub->short_ << "\n";
            }
        }

        return oss.str();
    }

    std::string buildFlagsSection() const {
        const auto [localFlags, globalFlags] = flagsForHelp();
        if (localFlags.empty()) return {};
        std::ostringstream oss;
        oss << "\nFlags:\n";
        for (const auto* f : localFlags) oss << "  " << formatFlagForHelp(*f) << "\n";
        (void)globalFlags;
        return oss.str();
    }

    std::string buildGlobalFlagsSection() const {
        const auto [localFlags, globalFlags] = flagsForHelp();
        if (globalFlags.empty()) return {};
        std::ostringstream oss;
        oss << "\nGlobal Flags:\n";
        for (const auto* f : globalFlags) oss << "  " << formatFlagForHelp(*f) << "\n";
        (void)localFlags;
        return oss.str();
    }

    static std::string renderTemplate(std::string_view tpl,
                                      const std::unordered_map<std::string, std::string>& vars) {
        std::string out;
        out.reserve(tpl.size());

        std::size_t i = 0;
        while (i < tpl.size()) {
            const auto start = tpl.find("{{.", i);
            if (start == std::string_view::npos) {
                out.append(tpl.substr(i));
                break;
            }
            out.append(tpl.substr(i, start - i));
            const auto end = tpl.find("}}", start);
            if (end == std::string_view::npos) {
                out.append(tpl.substr(start));
                break;
            }

            const auto key = tpl.substr(start + 3, end - (start + 3));
            const auto it = vars.find(std::string(key));
            if (it != vars.end()) out.append(it->second);
            i = end + 2;
        }
        return out;
    }

    Value* resolvedFlagValueBinding(const Flag& f) const {
        auto findBinding = [&](const std::string& name) -> Value* {
            const auto key = normalizeFlagName(name);
            for (auto* c = this; c; c = c->parent_) {
                auto it = c->flagValueBindings_.find(key);
                if (it != c->flagValueBindings_.end()) return it->second;
            }
            return nullptr;
        };

        if (!f.longName().empty()) {
            if (auto* v = findBinding(f.longName())) return v;
        }
        if (!f.shortName().empty()) {
            if (auto* v = findBinding(f.shortName())) return v;
        }
        return nullptr;
    }

    std::optional<std::string> flagTypeForHelp(const Flag& f) const {
        if (auto* v = resolvedFlagValueBinding(f)) {
            const auto t = v->type();
            if (!t.empty()) return t;
        }

        const auto& dv = f.defaultValue();
        if (std::holds_alternative<bool>(dv)) return std::nullopt; // Cobra-style: omit bool type.

        const auto ann = f.annotations();
        const auto bytesIt = ann.find("bytes");
        if (bytesIt != ann.end() && (bytesIt->second == "true" || bytesIt->second == "1" || bytesIt->second == "yes")) return "bytes";
        const auto it = ann.find("count");
        if (it != ann.end() && it->second == "true") return "count";
        const auto ipIt = ann.find("ip");
        if (ipIt != ann.end() && (ipIt->second == "true" || ipIt->second == "1" || ipIt->second == "yes")) return "ip";
        const auto ipMaskIt = ann.find("ipmask");
        if (ipMaskIt != ann.end() && (ipMaskIt->second == "true" || ipMaskIt->second == "1" || ipMaskIt->second == "yes")) return "ipmask";
        const auto cidrIt = ann.find("cidr");
        if (cidrIt != ann.end() && (cidrIt->second == "true" || cidrIt->second == "1" || cidrIt->second == "yes")) return "cidr";
        const auto ipNetIt = ann.find("ipnet");
        if (ipNetIt != ann.end() && (ipNetIt->second == "true" || ipNetIt->second == "1" || ipNetIt->second == "yes")) return "ipnet";
        const auto urlIt = ann.find("url");
        if (urlIt != ann.end() && (urlIt->second == "true" || urlIt->second == "1" || urlIt->second == "yes")) return "url";

        return std::visit(
            [](const auto& x) -> std::optional<std::string> {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, std::string>) return "string";
                if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return "duration";
                if constexpr (std::is_same_v<T, int>) return "int";
                if constexpr (std::is_same_v<T, std::int64_t>) return "int64";
                if constexpr (std::is_same_v<T, std::uint64_t>) return "uint64";
                if constexpr (std::is_same_v<T, float>) return "float32";
                if constexpr (std::is_same_v<T, double>) return "float64";
                return std::nullopt;
            },
            dv);
    }

    std::string formatFlagForHelp(const Flag& f) const {
        auto defaultForHelp = [](const Flag& flag) -> std::optional<std::string> {
            const auto& v = flag.defaultValue();

            // Cobra/pflag typically doesn't show "(default false)" for bool flags.
            if (std::holds_alternative<bool>(v)) {
                if (!std::get<bool>(v)) return std::nullopt;
                return "true";
            }

            return std::visit(
                [](const auto& x) -> std::optional<std::string> {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        if (x.empty()) return std::nullopt;
                        return std::string("\"") + escapeDoubleQuotes(x) + "\"";
                    } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
                        if (x.count() == 0) return std::nullopt;
                        return std::to_string(x.count()) + "ms";
                    } else {
                        std::ostringstream oss;
                        oss << x;
                        return oss.str();
                    }
                },
                v);
        };

        auto describeFlagForHelp = [&](const Flag& flag) -> std::string {
            std::string desc = flag.description();
            if (!flag.deprecated().empty()) desc += " (deprecated: " + flag.deprecated() + ")";
            if (flag.required()) desc += " (required)";
            if (auto def = defaultForHelp(flag)) {
                if (!desc.empty()) desc.push_back(' ');
                desc += "(default: " + *def + ")";
            }
            return desc;
        };

        std::string names;
        if (!f.shortName().empty()) names += f.shortName() + ", ";
        names += f.longName();

        if (auto ty = flagTypeForHelp(f)) {
            names.push_back(' ');
            names += *ty;
        }

        std::string desc = describeFlagForHelp(f);
        if (!desc.empty()) names += " - " + desc;
        return names;
    }

    std::pair<std::vector<const Flag*>, std::vector<const Flag*>> flagsForHelp() const {
        std::vector<const Flag*> local;
        std::vector<const Flag*> global;

        auto addVisible = [](std::vector<const Flag*>& dst, const std::vector<Flag>& src) {
            for (const auto& f : src) {
                if (f.hidden()) continue;
                dst.push_back(&f);
            }
        };

        addVisible(local, persistentFlags_);
        addVisible(local, flags_);

        const auto inherited = inheritedPersistentFlags();
        for (const auto* f : inherited) {
            if (f->hidden()) continue;
            global.push_back(f);
        }

        static const Flag helpFlag{"--help", "-h", "Help for this command"};
        local.push_back(&helpFlag);

        if (!resolvedVersion().empty()) {
            static const Flag versionFlag{"--version", "", "Version for this command"};
            global.push_back(&versionFlag);
        }

        if (!resolvedDisableSortFlags()) {
            auto flagLess = [](const Flag* a, const Flag* b) {
                const auto& an = a->longName().empty() ? a->shortName() : a->longName();
                const auto& bn = b->longName().empty() ? b->shortName() : b->longName();
                if (an != bn) return an < bn;
                return a->shortName() < b->shortName();
            };
            std::sort(local.begin(), local.end(), flagLess);
            std::sort(global.begin(), global.end(), flagLess);
        }

        return {local, global};
    }

    std::optional<std::string> checkRequiredFlags(const Parser& parser) const {
        const auto effective = effectiveFlags();
        for (const auto& f : effective) {
            if (!f.required()) continue;
            if (parser.hasValue(f.longName())) continue;
            return std::string("required flag not set: ") + f.longName();
        }
        return std::nullopt;
    }

    static std::string joinFlagNames(const std::vector<std::string>& names) {
        std::ostringstream oss;
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (i) oss << ", ";
            oss << names[i];
        }
        return oss.str();
    }

    template <typename GroupVec>
    static void collectFromChain(std::vector<std::vector<std::string>>& out, const GroupVec& groups) {
        for (const auto& g : groups) out.push_back(g);
    }

    std::optional<std::string> checkFlagGroups(const Parser& parser) const {
        std::vector<const Command*> chain;
        for (auto* c = this; c; c = c->parent_) chain.push_back(c);

        std::vector<std::vector<std::string>> mutexGroups;
        std::vector<std::vector<std::string>> oneRequiredGroups;
        std::vector<std::vector<std::string>> togetherGroups;

        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const auto* c = *it;
            collectFromChain(mutexGroups, c->mutuallyExclusiveFlagGroups_);
            collectFromChain(oneRequiredGroups, c->oneRequiredFlagGroups_);
            collectFromChain(togetherGroups, c->requiredTogetherFlagGroups_);
        }

        for (const auto& g : mutexGroups) {
            std::size_t setCount = 0;
            for (const auto& f : g) {
                if (parser.hasValue(f)) ++setCount;
                if (setCount >= 2) break;
            }
            if (setCount >= 2) {
                return std::string("flags are mutually exclusive: ") + joinFlagNames(g);
            }
        }

        for (const auto& g : oneRequiredGroups) {
            bool anySet = false;
            for (const auto& f : g) {
                if (parser.hasValue(f)) {
                    anySet = true;
                    break;
                }
            }
            if (!anySet) {
                return std::string("at least one of the flags in the group is required: ") + joinFlagNames(g);
            }
        }

        for (const auto& g : togetherGroups) {
            std::size_t setCount = 0;
            for (const auto& f : g) {
                if (parser.hasValue(f)) ++setCount;
            }
            if (setCount == 0 || setCount == g.size()) continue;
            return std::string("flags must be set together: ") + joinFlagNames(g);
        }

        return std::nullopt;
    }

    void warnDeprecatedFlags(const Parser& parser) const {
        const auto effective = effectiveFlags();
        for (const auto& f : effective) {
            if (f.deprecated().empty()) continue;
            if (!parser.hasFlag(f.longName())) continue;
            err() << "Flag \"" << f.longName() << "\" is deprecated: " << f.deprecated() << "\n";
        }
    }

    std::variant<int, std::string> runHooksAndAction(const Parser& parser, const std::vector<std::string>& args) {
        const auto chain = commandChain();

        for (auto* c : chain) {
            if (c->persistentPreRunE_) {
                if (auto err = c->persistentPreRunE_(*this, parser, args)) return *err;
            }
            if (c->persistentPreRun_) c->persistentPreRun_(*this, parser, args);
        }

        if (preRunE_) {
            if (auto err = preRunE_(*this, parser, args)) return *err;
        }
        if (preRun_) preRun_(*this, parser, args);

        int exitCode = 0;
        if (actionE_) {
            if (auto err = actionE_(*this, parser, args)) return *err;
        } else if (action_) {
            exitCode = action_(*this, parser, args);
        }

        if (postRunE_) {
            if (auto err = postRunE_(*this, parser, args)) return *err;
        }
        if (postRun_) postRun_(*this, parser, args);

        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            auto* c = *it;
            if (c->persistentPostRunE_) {
                if (auto err = c->persistentPostRunE_(*this, parser, args)) return *err;
            }
            if (c->persistentPostRun_) c->persistentPostRun_(*this, parser, args);
        }
        return exitCode;
    }

    std::vector<Command*> commandChain() {
        std::vector<Command*> chain;
        std::vector<Command*> tmp;
        for (auto* c = this; c; c = c->parent_) tmp.push_back(c);
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) chain.push_back(*it);
        return chain;
    }

    int fail(std::string message, bool showUsage) const {
        if (!silenceErrors_ && !message.empty()) {
            err() << "Error: " << message;
            if (message.back() != '\n') err() << "\n";
        }
        if (showUsage && !silenceUsage_) {
            err() << "\n";
            printUsageTo(err());
        }
        return 1;
    }

    int failUnknownCommand(const std::string& token) const {
        std::string msg = "unknown command \"" + token + "\" for \"" + commandPath() + "\"";
        if (resolvedSuggestions()) {
            const auto sugg = suggestCommands(token);
            if (!sugg.empty()) {
                msg += "\n\nDid you mean this?\n";
                for (const auto& s : sugg) msg += "  " + s + "\n";
            }
        }
        if (silenceErrors_) return 1;
        err() << "Error: " << msg << "\n";
        err() << "Run '" << commandPath() << " --help' for usage.\n";
        return 1;
    }

    void reparent(Command* parent) {
        parent_ = parent;
        for (auto& c : subcommands_) c->reparent(this);
    }

    struct CompletionEntry {
        std::string primaryPath;
        std::vector<std::string> pathAlternatives;
        std::vector<std::string> subcommands;
        std::vector<std::string> flags;
    };

    struct CompletionRequest {
        Command* cmd{nullptr};
        std::vector<std::string> argvForCmd;
    };

    struct CompletionItem {
        std::string value;
        std::string description;
    };

    std::vector<std::string> invocationNames() const;
    static std::string escapeDoubleQuotes(const std::string& s);
    static std::string joinWords(const std::vector<std::string>& words);
    static std::string joinQuotedPowerShell(const std::vector<std::string>& words);
    static std::string sanitizeIdentifier(const std::string& s);
    static std::string bashCaseLabel(const std::vector<std::string>& paths);
    std::vector<CompletionEntry> completionEntries() const;
    void collectCompletionEntries(std::vector<CompletionEntry>& entries, const std::vector<std::string>& paths) const;
    void printCompletionBashInternal(std::ostream& os, bool forZsh) const;

    CompletionRequest resolveForCompletion(const std::vector<std::string>& words) {
        CompletionRequest r;
        r.cmd = this;

        std::set<std::size_t> commandTokenIdx;
        bool positionalOnly = false;

        struct FlagInfo {
            bool isBool{false};
        };

        auto flagInfoLocal = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            const auto normalized = at->normalizeFlagKeyForLookup(key);
            const auto flags = at->effectiveFlags();
            for (const auto& f : flags) {
                if (f.longName() == normalized || f.shortName() == normalized) {
                    bool isBool = std::holds_alternative<bool>(f.defaultValue());
                    const auto it = f.annotations().find("count");
                    const bool isCount =
                        (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes"));
                    return FlagInfo{isBool || isCount};
                }
            }
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            return std::nullopt;
        };

        auto flagInfoSubtree = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            const auto normalized = at->normalizeFlagKeyForLookup(key);

            bool found = false;
            bool anyNonBool = false;

            std::function<void(const Command*)> visit = [&](const Command* c) {
                const auto flags = c->effectiveFlags();
                for (const auto& f : flags) {
                    if (f.longName() == normalized || f.shortName() == normalized) {
                        found = true;
                        bool isBool = std::holds_alternative<bool>(f.defaultValue());
                        const auto it = f.annotations().find("count");
                        const bool isCount =
                            (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes"));
                        if (!(isBool || isCount)) anyNonBool = true;
                        if (anyNonBool) return;
                    }
                }
                if (anyNonBool) return;
                for (const auto& child : c->subcommands_) visit(child.get());
            };

            visit(at);
            if (!found) return std::nullopt;
            return FlagInfo{!anyNonBool};
        };

        auto flagInfo = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (traverseChildren_) return flagInfoSubtree(at, key);
            return flagInfoLocal(at, key);
        };

        auto isFlagTokenLocal = [](const std::string& s) {
            return s.size() >= 2 && s[0] == '-' && s != "-";
        };

        auto isShortGroupToken = [](const std::string& s) {
            if (s.size() < 3) return false;
            if (s.rfind("--", 0) == 0) return false;
            if (s.find('=') != std::string::npos) return false;
            return s[0] == '-' && s[1] != '-';
        };

        auto skipFlagValueIfNeeded = [&](Command* at, const std::string& token, std::size_t& i) {
            const auto eq = token.find('=');
            if (eq != std::string::npos) return;

            if (at->resolvedBoolNegation() && token.rfind("--no-", 0) == 0) return;

            if (at->resolvedShortFlagGrouping() && isShortGroupToken(token)) {
                for (std::size_t pos = 1; pos < token.size(); ++pos) {
                    const std::string key = std::string("-") + token[pos];
                    const auto info = flagInfo(at, key);
                    if (!info.has_value()) return;
                    if (info->isBool) continue;
                    if (pos + 1 == token.size() && i + 1 < words.size()) ++i;
                    return;
                }
                return;
            }

            const auto info = flagInfo(at, token);
            if (!info.has_value()) return;
            if (info->isBool) return;
            if (i + 1 < words.size()) ++i;
        };

        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto& token = words[i];
            if (!positionalOnly && token == "--") {
                positionalOnly = true;
                continue;
            }
            if (positionalOnly) break;

            if (isFlagTokenLocal(token)) {
                skipFlagValueIfNeeded(r.cmd, token, i);
                continue;
            }

            if (auto* sub = r.cmd->findSubcommand(token)) {
                commandTokenIdx.insert(i);
                r.cmd = sub;
                continue;
            }
            break;
        }

        r.argvForCmd.reserve(words.size() + 1);
        r.argvForCmd.push_back(r.cmd->name_);
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (commandTokenIdx.count(i)) continue;
            r.argvForCmd.push_back(words[i]);
        }
        return r;
    }

    std::optional<std::string> resolveFlagNameForValueCompletion(const Command& at, std::string_view token) const {
        const auto eff = at.effectiveFlags();
        const std::string tok = at.normalizeFlagKeyForLookup(std::string(token));
        for (const auto& f : eff) {
            if (f.longName() == tok) return f.longName();
            if (!f.shortName().empty() && f.shortName() == tok && !f.longName().empty()) return f.longName();
        }
        return std::nullopt;
    }

    CompletionFunc* findFlagCompletionFunc(Command* at, const std::string& longName) {
        for (auto* c = at; c; c = c->parent_) {
            auto it = c->flagCompletionFuncs_.find(longName);
            if (it != c->flagCompletionFuncs_.end()) return &it->second;
        }
        return nullptr;
    }

    std::vector<CompletionItem> completeWords(const std::vector<std::string>& words,
                                              std::string_view toComplete,
                                              bool withDescriptions) {
        const auto ctx = resolveForCompletion(words);
        Command* cmd = ctx.cmd;

        std::vector<CompletionItem> out;
        std::uint32_t directive = cmd->resolvedCompletionDirective();

        auto parseDirectiveToken = [](std::string_view s) -> std::optional<std::uint32_t> {
            if (s.size() < 2 || s[0] != ':') return std::nullopt;
            std::uint32_t v = 0;
            for (std::size_t i = 1; i < s.size(); ++i) {
                const char ch = s[i];
                if (ch < '0' || ch > '9') return std::nullopt;
                v = static_cast<std::uint32_t>(v * 10 + static_cast<std::uint32_t>(ch - '0'));
            }
            return v;
        };

        auto maybeOverrideDirective = [&](std::string_view cand) -> bool {
            if (auto d = parseDirectiveToken(cand)) {
                directive = *d;
                return true;
            }
            return false;
        };

        const std::string toCompleteStr(toComplete);

        const bool completingFlagValueEq = (toCompleteStr.find('=') != std::string::npos);
        const bool completingFlagValueSeparate =
            (!words.empty() && (toCompleteStr.empty() || toCompleteStr[0] != '-') && words.back().rfind("-", 0) == 0);

        std::vector<std::string> argvVec = ctx.argvForCmd;
        if (completingFlagValueEq) {
            argvVec.push_back(toCompleteStr);
        } else if (completingFlagValueSeparate) {
            argvVec.push_back(toCompleteStr);
        }

        std::vector<char*> argvPtrs;
        argvPtrs.reserve(argvVec.size());
        for (auto& s : argvVec) argvPtrs.push_back(s.data());

        Parser::Options opts;
        opts.disableFlagParsing = cmd->disableFlagParsing_;
        opts.allowUnknownFlags = cmd->resolvedAllowUnknownFlags();
        opts.shortFlagGrouping = cmd->resolvedShortFlagGrouping();
        opts.boolNegation = cmd->resolvedBoolNegation();
        opts.suggestFlags = cmd->resolvedSuggestions();
        opts.suggestionsMinimumDistance = cmd->resolvedSuggestionsMinimumDistance();
        if (const auto* norm = cmd->resolvedNormalizeFlagKey()) opts.normalizeKey = *norm;
        const Parser parser(static_cast<int>(argvVec.size()), argvPtrs.data(), cmd->effectiveFlags(), opts);
        const auto& args = parser.positionals();

        // Value completion for --flag=prefix
        if (const auto eq = toCompleteStr.find('='); eq != std::string::npos) {
            const auto keyPart = toCompleteStr.substr(0, eq);
            const auto valPrefix = toCompleteStr.substr(eq + 1);
            if (auto longName = resolveFlagNameForValueCompletion(*cmd, keyPart)) {
                if (auto* func = findFlagCompletionFunc(cmd, *longName)) {
                    std::vector<std::string> raw = (*func)(*cmd, parser, args, valPrefix);
                    std::vector<std::string> cands;
                    cands.reserve(raw.size());
                    for (auto& cand : raw) {
                        if (maybeOverrideDirective(cand)) continue;
                        cands.push_back(std::move(cand));
                    }

                    const bool metadataList = ((directive & static_cast<std::uint32_t>(ShellCompDirective::FilterFileExt)) != 0);
                    for (const auto& cand : cands) {
                        if (!metadataList && cand.rfind(valPrefix, 0) != 0) continue;
                        if (metadataList) out.push_back({cand, {}});
                        else out.push_back({keyPart + "=" + cand, {}});
                    }
                }
            }
            out.push_back({":" + std::to_string(directive), {}});
            return out;
        }

        // Value completion for "--flag <prefix>" (toComplete may be empty when completing after a space).
        if (!words.empty() && (toCompleteStr.empty() || toCompleteStr[0] != '-')) {
            const auto& prev = words.back();
            if (prev.rfind("-", 0) == 0) {
                if (auto longName = resolveFlagNameForValueCompletion(*cmd, prev)) {
                    const auto eff = cmd->effectiveFlags();
                    bool expectsValue = false;
                    for (const auto& f : eff) {
                        if (f.longName() == *longName) {
                            expectsValue = !std::holds_alternative<bool>(f.defaultValue());
                            break;
                        }
                    }
                    if (expectsValue) {
                        if (auto* func = findFlagCompletionFunc(cmd, *longName)) {
                            std::vector<std::string> raw = (*func)(*cmd, parser, args, toComplete);
                            std::vector<std::string> cands;
                            cands.reserve(raw.size());
                            for (auto& cand : raw) {
                                if (maybeOverrideDirective(cand)) continue;
                                cands.push_back(std::move(cand));
                            }

                            const bool metadataList = ((directive & static_cast<std::uint32_t>(ShellCompDirective::FilterFileExt)) != 0);
                            for (const auto& cand : cands) {
                                if (!metadataList && cand.rfind(toCompleteStr, 0) != 0) continue;
                                out.push_back({cand, {}});
                            }
                        }
                        out.push_back({":" + std::to_string(directive), {}});
                        return out;
                    }
                }
            }
        }

        // Flag name completion.
        if (!toCompleteStr.empty() && toCompleteStr[0] == '-') {
            const auto eff = cmd->effectiveFlags();
            std::vector<CompletionItem> cands;
            cands.reserve(eff.size() * 2 + 3);

            auto addFlag = [&](const std::string& name, const std::string& desc) {
                if (name.empty()) return;
                if (name.rfind(toCompleteStr, 0) != 0) return;
                cands.push_back({name, withDescriptions ? desc : ""});
            };

            for (const auto& f : eff) {
                if (f.hidden()) continue;
                addFlag(f.longName(), f.description());
                addFlag(f.shortName(), f.description());
            }
            addFlag("--help", "Help for this command");
            addFlag("-h", "Help for this command");
            if (!cmd->resolvedVersion().empty()) addFlag("--version", "Version for this command");

            out.insert(out.end(), cands.begin(), cands.end());
            out.push_back({":" + std::to_string(directive), {}});
            return out;
        }

        // Subcommands.
        {
            std::unordered_set<std::string> seen;
            for (const auto* sub : cmd->listVisibleSubcommands()) {
                for (const auto& name : sub->invocationNames()) {
                    if (name.rfind(toCompleteStr, 0) != 0) continue;
                    if (!seen.insert(name).second) continue;
                    out.push_back({name, withDescriptions ? sub->short_ : ""});
                }
            }
            if (cmd->isRoot()) {
                if (cmd->resolvedAddHelpCommand()) {
                    const auto helpName = cmd->resolvedHelpCommandName();
                    if (helpName.rfind(toCompleteStr, 0) == 0) {
                        out.push_back({helpName, withDescriptions ? "Help about any command" : ""});
                    }
                }
                if (!cmd->resolvedVersion().empty() && std::string("version").rfind(toCompleteStr, 0) == 0) {
                    out.push_back({"version", withDescriptions ? "Print the version number" : ""});
                }
            }
        }

        // Valid args / dynamic args completion.
        if (cmd->validArgsFunction_) {
            for (auto& cand : cmd->validArgsFunction_(*cmd, parser, args, toComplete)) {
                if (maybeOverrideDirective(cand)) continue;
                if (cand.rfind(toCompleteStr, 0) != 0) continue;
                out.push_back({cand, {}});
            }
        } else {
            for (const auto& cand : cmd->validArgs_) {
                if (maybeOverrideDirective(cand)) continue;
                if (cand.rfind(toCompleteStr, 0) != 0) continue;
                out.push_back({cand, {}});
            }
        }

        out.push_back({":" + std::to_string(directive), {}});
        return out;
    }

    std::unordered_map<std::string, std::string> effectiveEnvBindings() const;
    std::optional<std::string> applyExternalSources(Parser& parser) const;
    std::optional<std::string> applyBoundFlagValues(const Parser& parser);

    std::vector<std::string> suggestCommands(const std::string& token) const {
        struct Scored {
            std::string value;
            std::size_t score;
        };

        std::vector<Scored> scored;
        for (const auto* c : listVisibleSubcommands()) {
            std::size_t best = (c->name_.rfind(token, 0) == 0) ? 0 : utils::levenshteinDistance(token, c->name_);
            for (const auto& a : c->aliases_) {
                const std::size_t s = (a.rfind(token, 0) == 0) ? 0 : utils::levenshteinDistance(token, a);
                if (s < best) best = s;
            }
            scored.push_back({c->name_, best});
        }

        std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
            if (a.score != b.score) return a.score < b.score;
            return a.value < b.value;
        });

        std::vector<std::string> out;
        for (const auto& s : scored) {
            if (out.size() >= 3) break;
            if (s.score <= resolvedSuggestionsMinimumDistance()) out.push_back(s.value);
        }
        return out;
    }

    Resolution resolveForExecution(int argc, char** argv) {
        Resolution r;
        r.cmd = this;

        const auto helpName = resolvedHelpCommandName();
        const bool helpEnabled = resolvedAddHelpCommand();

        std::set<int> commandTokenIdx;
        bool positionalOnly = false;

        struct FlagInfo {
            bool isBool{false};
        };

        auto flagInfoLocal = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            const auto normalized = at->normalizeFlagKeyForLookup(key);
            const auto flags = at->effectiveFlags();
            for (const auto& f : flags) {
                if (f.longName() == normalized || f.shortName() == normalized) {
                    bool isBool = std::holds_alternative<bool>(f.defaultValue());
                    const auto it = f.annotations().find("count");
                    const bool isCount =
                        (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes"));
                    return FlagInfo{isBool || isCount};
                }
            }
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            return std::nullopt;
        };

        auto flagInfoSubtree = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            const auto normalized = at->normalizeFlagKeyForLookup(key);

            bool found = false;
            bool anyNonBool = false;

            std::function<void(const Command*)> visit = [&](const Command* c) {
                const auto flags = c->effectiveFlags();
                for (const auto& f : flags) {
                    if (f.longName() == normalized || f.shortName() == normalized) {
                        found = true;
                        bool isBool = std::holds_alternative<bool>(f.defaultValue());
                        const auto it = f.annotations().find("count");
                        const bool isCount =
                            (it != f.annotations().end() && (it->second == "true" || it->second == "1" || it->second == "yes"));
                        if (!(isBool || isCount)) anyNonBool = true;
                        if (anyNonBool) return;
                    }
                }
                if (anyNonBool) return;
                for (const auto& child : c->subcommands_) visit(child.get());
            };

            visit(at);
            if (!found) return std::nullopt;
            return FlagInfo{!anyNonBool};
        };

        auto flagInfo = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (traverseChildren_) return flagInfoSubtree(at, key);
            return flagInfoLocal(at, key);
        };

        auto skipFlagValueIfNeeded = [&](Command* at, const std::string& token, int& i) {
            const auto eq = token.find('=');
            if (eq != std::string::npos) return;

            if (at->resolvedBoolNegation() && token.rfind("--no-", 0) == 0) return;

            if (at->resolvedShortFlagGrouping() && token.size() >= 3 && token[0] == '-' && token[1] != '-') {
                // Short group: -abc
                for (std::size_t pos = 1; pos < token.size(); ++pos) {
                    const std::string key = std::string("-") + token[pos];
                    const auto info = flagInfo(at, key);
                    if (!info.has_value()) return;
                    if (info->isBool) continue;
                    // Value is either the remainder of this token (-ovalue) or the next arg (-o value).
                    if (pos + 1 == token.size() && i + 1 < argc) ++i;
                    return;
                }
                return;
            }

            const auto info = flagInfo(at, token);
            if (!info.has_value()) return;
            if (info->isBool) return;
            if (i + 1 < argc) ++i;
        };

        for (int i = 1; i < argc; ++i) {
            std::string token = argv[i];
            if (!positionalOnly && token == "--") {
                positionalOnly = true;
                continue;
            }
            if (positionalOnly) break;

            if (isFlagToken(token)) {
                skipFlagValueIfNeeded(r.cmd, token, i);
                continue;
            }

            if (helpEnabled && token == helpName && r.cmd == this) {
                r.helpRequested = true;
                r.helpBase = this;
                for (int j = i + 1; j < argc; ++j) {
                    std::string t = argv[j];
                    if (t == "--") break;
                    if (isFlagToken(t)) break;
                    r.helpPath.push_back(std::move(t));
                }
                return r;
            }
            if (token == "version" && r.cmd == this) {
                r.versionRequested = true;
                return r;
            }

            if (auto* sub = r.cmd->findSubcommand(token)) {
                commandTokenIdx.insert(i);
                r.cmd = sub;
                continue;
            }
            break;
        }

        r.argvForCmd.reserve(static_cast<std::size_t>(argc));
        r.argvForCmd.push_back(r.cmd->name_);
        for (int i = 1; i < argc; ++i) {
            if (commandTokenIdx.count(i)) continue;
            r.argvForCmd.push_back(argv[i]);
        }
        return r;
    }

    int execHelp(Command& base, const std::vector<std::string>& path) const {
        const Command* target = &base;
        for (const auto& name : path) {
            if (auto* sub = target->findSubcommand(name)) {
                target = sub;
                continue;
            }
            base.failUnknownCommand(name);
            return 1;
        }
        target->printHelp();
        return 0;
    }

    std::string name_;
    std::string short_;
    std::string long_;
    std::vector<std::string> aliases_;
    std::unordered_map<std::string, std::string> annotations_;
    std::vector<CommandGroup> groups_;
    std::string groupId_;
    std::vector<Flag> flags_;
    std::vector<Flag> persistentFlags_;
    std::vector<std::unique_ptr<Command>> subcommands_;
    Action action_;
    ActionE actionE_;
    ArgsValidator args_;
    Hook preRun_;
    HookE preRunE_;
    Hook postRun_;
    HookE postRunE_;
    Hook persistentPreRun_;
    HookE persistentPreRunE_;
    Hook persistentPostRun_;
    HookE persistentPostRunE_;
    std::string version_;
    Command* parent_{nullptr};
    bool hidden_{false};
    std::string deprecated_;
    std::string example_;
    bool silenceUsage_{false};
    bool silenceErrors_{false};
    bool suggestions_{true};
    std::size_t suggestionsMinimumDistance_{2};
    std::optional<bool> suggestionsOverride_;
    std::optional<std::size_t> suggestionsMinimumDistanceOverride_;
    std::optional<bool> disableSortCommandsOverride_;
    std::optional<bool> disableSortFlagsOverride_;
    std::optional<bool> disableFlagsInUseLineOverride_;
    std::optional<bool> addHelpCommandOverride_;
    std::optional<std::string> helpCommandNameOverride_;
    std::optional<std::string> helpTemplateOverride_;
    std::optional<std::string> usageTemplateOverride_;
    std::optional<std::string> versionTemplateOverride_;
    std::optional<HelpFunc> helpFuncOverride_;
    std::optional<UsageFunc> usageFuncOverride_;
    std::optional<FlagErrorFunc> flagErrorFuncOverride_;
    std::optional<bool> allowUnknownFlagsOverride_;
    std::optional<bool> shortFlagGroupingOverride_;
    std::optional<bool> boolNegationOverride_;
    std::optional<NormalizeKeyFunc> normalizeFlagKeyOverride_;
    std::optional<std::uint32_t> completionDirectiveOverride_;
    std::optional<CompletionConfig> completionConfigOverride_;
    bool disableFlagParsing_{false};
    bool traverseChildren_{false};
    std::vector<std::vector<std::string>> mutuallyExclusiveFlagGroups_;
    std::vector<std::vector<std::string>> oneRequiredFlagGroups_;
    std::vector<std::vector<std::string>> requiredTogetherFlagGroups_;
    std::vector<std::string> validArgs_;
    CompletionFunc validArgsFunction_;
    std::unordered_map<std::string, CompletionFunc> flagCompletionFuncs_;
    std::unordered_map<std::string, Value*> flagValueBindings_;
    std::unordered_map<std::string, std::string> envBindings_;
    std::string configFilePath_;
    std::string configFileFlag_;
    std::optional<std::vector<std::string>> argsOverride_;
    std::optional<std::any> contextOverride_;
    std::ostream* out_{nullptr};
    std::ostream* err_{nullptr};
};

inline std::vector<std::string> Command::invocationNames() const {
    std::vector<std::string> out;
    out.push_back(name_);
    out.insert(out.end(), aliases_.begin(), aliases_.end());
    return out;
}

inline std::string Command::escapeDoubleQuotes(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const char ch : s) {
        if (ch == '"') out.push_back('\\');
        out.push_back(ch);
    }
    return out;
}

inline std::string Command::joinWords(const std::vector<std::string>& words) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (i) oss << ' ';
        oss << words[i];
    }
    return oss.str();
}

inline std::string Command::joinQuotedPowerShell(const std::vector<std::string>& words) {
    auto escapeSingleQuoted = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (const char ch : s) {
            if (ch == '\'') out += "''";
            else out.push_back(ch);
        }
        return out;
    };

    std::ostringstream oss;
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (i) oss << ", ";
        oss << "'" << escapeSingleQuoted(words[i]) << "'";
    }
    return oss.str();
}

inline std::string Command::sanitizeIdentifier(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const unsigned char ch : s) {
        if (std::isalnum(ch) || ch == '_') out.push_back(static_cast<char>(ch));
        else out.push_back('_');
    }
    if (out.empty() || (!std::isalpha(static_cast<unsigned char>(out[0])) && out[0] != '_')) out.insert(out.begin(), '_');
    return out;
}

inline std::string Command::bashCaseLabel(const std::vector<std::string>& paths) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (i) oss << "|";
        oss << "\"" << escapeDoubleQuotes(paths[i]) << "\"";
    }
    return oss.str();
}

inline std::vector<Command::CompletionEntry> Command::completionEntries() const {
    std::vector<CompletionEntry> entries;
    collectCompletionEntries(entries, {name_});

    if (resolvedAddHelpCommand()) {
        const auto helpName = resolvedHelpCommandName();
        CompletionEntry helpEntry;
        helpEntry.primaryPath = name_ + " " + helpName;
        helpEntry.pathAlternatives = {helpEntry.primaryPath};
        for (const auto* sub : listVisibleSubcommands()) {
            helpEntry.subcommands.push_back(sub->name_);
            for (const auto& a : sub->aliases_) helpEntry.subcommands.push_back(a);
        }
        helpEntry.flags = {"--help", "-h"};
        if (!resolvedVersion().empty()) helpEntry.flags.push_back("--version");
        entries.push_back(std::move(helpEntry));
    }

    if (!resolvedVersion().empty()) {
        CompletionEntry versionEntry;
        versionEntry.primaryPath = name_ + " version";
        versionEntry.pathAlternatives = {versionEntry.primaryPath};
        versionEntry.flags = {"--help", "-h"};
        entries.push_back(std::move(versionEntry));
    }

    return entries;
}

inline void Command::collectCompletionEntries(std::vector<CompletionEntry>& entries,
                                              const std::vector<std::string>& paths) const {
    CompletionEntry e;
    e.primaryPath = paths.front();
    e.pathAlternatives = paths;

    for (const auto* sub : listVisibleSubcommands()) {
        e.subcommands.push_back(sub->name_);
        for (const auto& a : sub->aliases_) e.subcommands.push_back(a);
    }
    if (isRoot()) {
        if (resolvedAddHelpCommand()) e.subcommands.push_back(resolvedHelpCommandName());
        if (!resolvedVersion().empty()) e.subcommands.push_back("version");
    }

    const auto eff = effectiveFlags();
    for (const auto& f : eff) {
        if (!f.longName().empty()) e.flags.push_back(f.longName());
        if (!f.shortName().empty()) e.flags.push_back(f.shortName());
    }
    e.flags.push_back("--help");
    e.flags.push_back("-h");
    if (!resolvedVersion().empty()) e.flags.push_back("--version");

    entries.push_back(std::move(e));

    for (const auto* sub : listVisibleSubcommands()) {
        std::vector<std::string> childPaths;
        const auto inv = sub->invocationNames();
        childPaths.reserve(paths.size() * inv.size());
        for (const auto& p : paths) {
            for (const auto& n : inv) childPaths.push_back(p + " " + n);
        }
        sub->collectCompletionEntries(entries, childPaths);
    }
}

inline void Command::printCompletionBashInternal(std::ostream& os, bool forZsh) const {
    const auto rootName = name_;
    const auto fn = "_" + sanitizeIdentifier(rootName) + "_complete";

    const auto cfg = resolvedCompletionConfig();
    const bool dynamicOk = cfg.addCompleteCommands && !cfg.completeNoDescCommandName.empty();

    if (forZsh) os << "#compdef " << rootName << "\n\nautoload -U +X bashcompinit && bashcompinit\n\n";
    os << "# bash completion for " << rootName << "\n";

    if (!dynamicOk) {
        // Fallback: keep a static completion script.
        const auto entries = completionEntries();
        os << fn << "() {\n";
        os << "  local cur cmd subcommands flags i w\n";
        os << "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
        os << "  cmd=\"" << rootName << "\"\n";
        os << "  for ((i=1; i<COMP_CWORD; i++)); do\n";
        os << "    w=\"${COMP_WORDS[i]}\"\n";
        os << "    [[ \"$w\" == -* ]] && continue\n";
        os << "    subcommands=\"\"\n";
        os << "    case \"$cmd\" in\n";
        for (const auto& e : entries) {
            os << "      " << bashCaseLabel(e.pathAlternatives) << ")\n";
            os << "        subcommands=\"" << joinWords(e.subcommands) << "\"\n";
            os << "        ;;\n";
        }
        os << "    esac\n";
        os << "    if [[ \" $subcommands \" == *\" $w \"* ]]; then\n";
        os << "      cmd=\"$cmd $w\"\n";
        os << "    else\n";
        os << "      break\n";
        os << "    fi\n";
        os << "  done\n";
        os << "  subcommands=\"\"; flags=\"\"\n";
        os << "  case \"$cmd\" in\n";
        for (const auto& e : entries) {
            os << "    " << bashCaseLabel(e.pathAlternatives) << ")\n";
            os << "      subcommands=\"" << joinWords(e.subcommands) << "\"\n";
            os << "      flags=\"" << joinWords(e.flags) << "\"\n";
            os << "      ;;\n";
        }
        os << "  esac\n";
        os << "  if [[ \"$cur\" == -* ]]; then\n";
        os << "    COMPREPLY=( $(compgen -W \"$flags\" -- \"$cur\") )\n";
        os << "  else\n";
        os << "    COMPREPLY=( $(compgen -W \"$subcommands\" -- \"$cur\") )\n";
        os << "  fi\n";
        os << "}\n";
        os << "complete -F " << fn << " " << rootName << "\n";
        return;
    }

    // Dynamic: call the binary's internal __completeNoDesc handler.
    os << fn << "() {\n";
    os << "  local cur i line directive lastIndex out words eqflag prefix\n";
    os << "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
    os << "  if (( COMP_CWORD > 1 )); then\n";
    os << "    words=(\"${COMP_WORDS[@]:1:$((COMP_CWORD-1))}\")\n";
    os << "  else\n";
    os << "    words=()\n";
    os << "  fi\n";
    os << "  eqflag=\"\"; prefix=\"\"\n";
    os << "  if [[ \"$cur\" == -*=* ]]; then\n";
    os << "    eqflag=\"${cur%%=*}\"\n";
    os << "    cur=\"${cur#*=}\"\n";
    os << "    words+=(\"$eqflag\")\n";
    os << "    prefix=\"$eqflag=\"\n";
    os << "  fi\n";
    os << "  out=()\n";
    os << "  while IFS= read -r line; do\n";
    os << "    out+=(\"$line\")\n";
    os << "  done < <(" << rootName << " " << cfg.completeNoDescCommandName << " \"${words[@]}\" \"$cur\" 2>/dev/null)\n";
    os << "  if [[ ${#out[@]} -eq 0 ]]; then\n";
    os << "    return 0\n";
    os << "  fi\n";
    os << "  lastIndex=$((${#out[@]}-1))\n";
    os << "  directive=\"${out[$lastIndex]}\"\n";
    os << "  unset \"out[$lastIndex]\"\n";
    os << "  if [[ \"$directive\" =~ ^:([0-9]+)$ ]]; then\n";
    os << "    directive=\"${BASH_REMATCH[1]}\"\n";
    os << "  else\n";
    os << "    directive=0\n";
    os << "  fi\n";
    os << "  COMPREPLY=()\n";
    os << "  # ShellCompDirectiveFilterDirs (16)\n";
    os << "  if (( (directive & 16) != 0 )); then\n";
    os << "    COMPREPLY=( $(compgen -d -- \"$cur\") )\n";
    os << "  # ShellCompDirectiveFilterFileExt (8)\n";
    os << "  elif (( (directive & 8) != 0 )); then\n";
    os << "    local ext\n";
    os << "    local -a matches\n";
    os << "    matches=()\n";
    os << "    for ext in \"${out[@]}\"; do\n";
    os << "      ext=\"${ext#.}\"\n";
    os << "      [[ -z \"$ext\" ]] && continue\n";
    os << "      while IFS= read -r line; do\n";
    os << "        matches+=(\"$line\")\n";
    os << "      done < <(compgen -f -X \"!*.$ext\" -- \"$cur\")\n";
    os << "    done\n";
    os << "    COMPREPLY=(\"${matches[@]}\")\n";
    os << "  else\n";
    os << "    for line in \"${out[@]}\"; do\n";
    os << "      [[ \"$line\" == \"$cur\"* ]] && COMPREPLY+=(\"$line\")\n";
    os << "    done\n";
    os << "  fi\n";
    os << "  if [[ -n \"$prefix\" ]]; then\n";
    os << "    for i in \"${!COMPREPLY[@]}\"; do\n";
    os << "      COMPREPLY[$i]=\"$prefix${COMPREPLY[$i]}\"\n";
    os << "    done\n";
    os << "  fi\n";
    os << "  # ShellCompDirectiveNoSpace (2)\n";
    os << "  if (( (directive & 2) != 0 )); then\n";
    os << "    compopt -o nospace 2>/dev/null\n";
    os << "  fi\n";
    os << "  # ShellCompDirectiveKeepOrder (32)\n";
    os << "  if (( (directive & 32) != 0 )); then\n";
    os << "    compopt -o nosort 2>/dev/null\n";
    os << "  fi\n";
    os << "  # ShellCompDirectiveNoFileComp (4) OR any explicit file-mode directive.\n";
    os << "  if (( (directive & 4) != 0 || (directive & 8) != 0 || (directive & 16) != 0 )); then\n";
    os << "    compopt +o default 2>/dev/null\n";
    os << "  fi\n";
    os << "  # ShellCompDirectiveError (1)\n";
    os << "  if (( (directive & 1) != 0 )); then\n";
    os << "    return 1\n";
    os << "  fi\n";
    os << "}\n";
    os << "complete -F " << fn << " " << rootName << "\n";
}

inline std::unordered_map<std::string, std::string> Command::effectiveEnvBindings() const {
    std::unordered_map<std::string, std::string> out;
    std::vector<const Command*> chain;
    for (auto* c = this; c; c = c->parent_) chain.push_back(c);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        for (const auto& [flag, env] : (*it)->envBindings_) out[flag] = env;
    }
    return out;
}

inline std::optional<std::string> Command::applyBoundFlagValues(const Parser& parser) {
    auto resolved = [&](const std::string& name) -> Value* {
        const auto key = normalizeFlagName(name);
        for (auto* c = this; c; c = c->parent_) {
            auto it = c->flagValueBindings_.find(key);
            if (it != c->flagValueBindings_.end()) return it->second;
        }
        return nullptr;
    };

    const auto effFlags = effectiveFlags();
    for (const auto& f : effFlags) {
        if (f.longName().empty()) continue;
        Value* v = resolved(f.longName());
        if (!v && !f.shortName().empty()) v = resolved(f.shortName());
        if (!v) continue;

        if (!parser.hasExplicitValue(f.longName())) continue;
        const auto values = parser.getExplicitFlagValues(f.longName());
        for (const auto& s : values) {
            if (auto err = v->set(s)) {
                return *err;
            }
        }
    }
    return std::nullopt;
}

inline std::optional<std::string> Command::applyExternalSources(Parser& parser) const {
    std::unordered_map<std::string, std::string> external;
    std::unordered_map<std::string, std::vector<std::string>> externalMulti;

    auto trim = [](std::string_view s) -> std::string_view {
        std::size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        std::size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
    };

    auto endsWith = [](std::string_view s, std::string_view suffix) {
        if (suffix.size() > s.size()) return false;
        return s.substr(s.size() - suffix.size()) == suffix;
    };

    auto parseEnvLike = [&](std::istream& in) -> std::unordered_map<std::string, std::string> {
        std::unordered_map<std::string, std::string> raw;
        std::string line;
        while (std::getline(in, line)) {
            const auto t = trim(line);
            if (t.empty()) continue;
            if (t.front() == '#') continue;
            const auto eq = t.find('=');
            if (eq == std::string_view::npos) continue;
            const auto key = trim(t.substr(0, eq));
            auto value = std::string(trim(t.substr(eq + 1)));
            if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }
            raw[std::string(key)] = std::move(value);
        }
        return raw;
    };

    struct FlatRaw {
        std::unordered_map<std::string, std::string> scalar;
        std::unordered_map<std::string, std::vector<std::string>> multi;
    };

    struct JsonParse {
        const char* p{nullptr};
        const char* end{nullptr};

        static bool isWs(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

        void skipWs() {
            while (p < end && isWs(*p)) ++p;
        }

        bool consume(char c) {
            skipWs();
            if (p >= end || *p != c) return false;
            ++p;
            return true;
        }

        std::optional<std::string> parseString() {
            skipWs();
            if (p >= end || *p != '"') return std::nullopt;
            ++p;
            std::string out;
            while (p < end) {
                const char ch = *p++;
                if (ch == '"') return out;
                if (ch != '\\') {
                    out.push_back(ch);
                    continue;
                }
                if (p >= end) return std::nullopt;
                const char esc = *p++;
                switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default:
                    // Minimal parser: unsupported escapes fail.
                    return std::nullopt;
                }
            }
            return std::nullopt;
        }

        std::optional<std::string> parseNumberToken() {
            skipWs();
            if (p >= end) return std::nullopt;
            const char* start = p;
            if (*p == '-' || *p == '+') ++p;
            bool any = false;
            while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
                any = true;
                ++p;
            }
            if (p < end && *p == '.') {
                ++p;
                while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
                    any = true;
                    ++p;
                }
            }
            if (!any) return std::nullopt;
            if (p < end && (*p == 'e' || *p == 'E')) {
                ++p;
                if (p < end && (*p == '+' || *p == '-')) ++p;
                bool expAny = false;
                while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
                    expAny = true;
                    ++p;
                }
                if (!expAny) return std::nullopt;
            }
            return std::string(start, p);
        }

        bool startsWith(std::string_view lit) const {
            return static_cast<std::size_t>(end - p) >= lit.size() && std::string_view(p, lit.size()) == lit;
        }

        std::optional<std::string> parseScalarToken() {
            skipWs();
            if (p >= end) return std::nullopt;
            if (*p == '"') return parseString();
            if (*p == '-' || *p == '+' || std::isdigit(static_cast<unsigned char>(*p))) return parseNumberToken();
            if (startsWith("true")) {
                p += 4;
                return std::string("true");
            }
            if (startsWith("false")) {
                p += 5;
                return std::string("false");
            }
            if (startsWith("null")) {
                p += 4;
                return std::string();
            }
            return std::nullopt;
        }

        std::optional<std::vector<std::string>> parseArrayScalarTokens() {
            if (!consume('[')) return std::nullopt;
            skipWs();
            std::vector<std::string> out;
            if (consume(']')) return out;
            while (true) {
                skipWs();
                if (p >= end) return std::nullopt;
                if (*p == '{' || *p == '[') return std::nullopt;
                auto v = parseScalarToken();
                if (!v.has_value()) return std::nullopt;
                out.push_back(*v);
                skipWs();
                if (consume(']')) return out;
                if (!consume(',')) return std::nullopt;
            }
        }

        bool skipValue() {
            skipWs();
            if (p >= end) return false;
            if (*p == '{') return skipObject();
            if (*p == '[') return skipArray();
            return static_cast<bool>(parseScalarToken());
        }

        bool skipArray() {
            if (!consume('[')) return false;
            skipWs();
            if (consume(']')) return true;
            while (true) {
                if (!skipValue()) return false;
                skipWs();
                if (consume(']')) return true;
                if (!consume(',')) return false;
            }
        }

        bool skipObject() {
            if (!consume('{')) return false;
            skipWs();
            if (consume('}')) return true;
            while (true) {
                if (!parseString().has_value()) return false;
                if (!consume(':')) return false;
                if (!skipValue()) return false;
                skipWs();
                if (consume('}')) return true;
                if (!consume(',')) return false;
            }
        }

        bool parseObjectFlatten(FlatRaw& out, const std::string& prefix) {
            if (!consume('{')) return false;
            skipWs();
            if (consume('}')) return true;
            while (true) {
                auto key = parseString();
                if (!key.has_value()) return false;
                if (!consume(':')) return false;
                skipWs();
                const std::string fullKey = prefix.empty() ? *key : (prefix + "." + *key);

                if (p < end && *p == '{') {
                    if (!parseObjectFlatten(out, fullKey)) return false;
                } else if (p < end && *p == '[') {
                    const char* checkpoint = p;
                    if (auto vec = parseArrayScalarTokens()) {
                        if (!vec->empty()) out.multi[fullKey] = std::move(*vec);
                    } else {
                        p = checkpoint;
                        // Not a scalar array, skip without failing.
                        if (!skipArray()) return false;
                    }
                } else {
                    auto val = parseScalarToken();
                    if (!val.has_value()) return false;
                    out.scalar[fullKey] = *val;
                }

                skipWs();
                if (consume('}')) return true;
                if (!consume(',')) return false;
            }
        }
    };

    auto parseJsonFlatten = [&](std::istream& in) -> std::optional<FlatRaw> {
        std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        JsonParse jp;
        jp.p = contents.data();
        jp.end = contents.data() + contents.size();
        jp.skipWs();
        FlatRaw out;
        if (!jp.parseObjectFlatten(out, /*prefix=*/"")) return std::nullopt;
        jp.skipWs();
        if (jp.p != jp.end) return std::nullopt;
        return out;
    };

    auto parseTomlFlatten = [&](std::istream& in) -> std::optional<FlatRaw> {
        FlatRaw out;
        std::string tablePrefix;

        auto stripComment = [](std::string_view s) -> std::string_view {
            bool inDouble = false;
            bool inSingle = false;
            for (std::size_t i = 0; i < s.size(); ++i) {
                const char ch = s[i];
                if (!inSingle && ch == '"' && (i == 0 || s[i - 1] != '\\')) inDouble = !inDouble;
                else if (!inDouble && ch == '\'') inSingle = !inSingle;
                else if (!inDouble && !inSingle && ch == '#') return s.substr(0, i);
            }
            return s;
        };

        auto unquote = [](std::string_view v) -> std::string {
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
                std::string out;
                out.reserve(v.size() - 2);
                for (std::size_t i = 1; i + 1 < v.size(); ++i) {
                    char ch = v[i];
                    if (ch == '\\' && i + 2 < v.size()) {
                        const char n = v[++i];
                        switch (n) {
                        case 'n': out.push_back('\n'); break;
                        case 't': out.push_back('\t'); break;
                        case 'r': out.push_back('\r'); break;
                        case '\\': out.push_back('\\'); break;
                        case '"': out.push_back('"'); break;
                        default: out.push_back(n); break;
                        }
                        continue;
                    }
                    out.push_back(ch);
                }
                return out;
            }
            if (v.size() >= 2 && v.front() == '\'' && v.back() == '\'') {
                return std::string(v.substr(1, v.size() - 2));
            }
            return std::string(v);
        };

        auto parseTomlArray = [&](std::string_view v) -> std::optional<std::vector<std::string>> {
            v = trim(v);
            if (v.size() < 2 || v.front() != '[' || v.back() != ']') return std::nullopt;
            v = trim(v.substr(1, v.size() - 2));
            std::vector<std::string> out;
            std::size_t i = 0;
            while (i < v.size()) {
                while (i < v.size() && std::isspace(static_cast<unsigned char>(v[i]))) ++i;
                if (i >= v.size()) break;
                std::size_t start = i;
                bool inDouble = false;
                bool inSingle = false;
                while (i < v.size()) {
                    const char ch = v[i];
                    if (!inSingle && ch == '"' && (i == start || v[i - 1] != '\\')) inDouble = !inDouble;
                    else if (!inDouble && ch == '\'') inSingle = !inSingle;
                    else if (!inDouble && !inSingle && ch == ',') break;
                    ++i;
                }
                auto item = trim(v.substr(start, i - start));
                if (!item.empty()) out.push_back(unquote(item));
                if (i < v.size() && v[i] == ',') ++i;
            }
            return out;
        };

        std::string line;
        while (std::getline(in, line)) {
            const auto t = trim(line);
            if (t.empty()) continue;
            if (t.front() == '#') continue;

            if (t.front() == '[') {
                // Table header: [a.b]
                const auto close = t.find(']');
                if (close == std::string_view::npos) return std::nullopt;
                const auto inner = trim(t.substr(1, close - 1));
                if (inner.empty()) {
                    tablePrefix.clear();
                } else {
                    tablePrefix = std::string(inner);
                }
                continue;
            }

            const auto eq = t.find('=');
            if (eq == std::string_view::npos) continue;
            const auto key = trim(t.substr(0, eq));
            auto valueView = trim(t.substr(eq + 1));
            valueView = trim(stripComment(valueView));

            if (key.empty()) continue;
            if (valueView.empty()) {
                // Preserve empty value as explicit override.
            }

            const std::string fullKey = tablePrefix.empty() ? std::string(key) : (tablePrefix + "." + std::string(key));
            if (!valueView.empty() && valueView.front() == '[') {
                if (auto vec = parseTomlArray(valueView)) {
                    if (!vec->empty()) out.multi[fullKey] = std::move(*vec);
                    continue;
                }
                // Unknown array type; skip.
                continue;
            }
            if (!valueView.empty() && valueView.front() == '{') {
                // Inline table not supported.
                continue;
            }
            out.scalar[fullKey] = unquote(valueView);
        }

        return out;
    };

    auto parseYamlFlatten = [&](std::istream& in) -> std::optional<FlatRaw> {
        FlatRaw out;

        struct Frame {
            std::size_t indent;
            std::string key;
        };
        std::vector<Frame> stack;

        auto stripComment = [](std::string_view s) -> std::string_view {
            bool inDouble = false;
            bool inSingle = false;
            for (std::size_t i = 0; i < s.size(); ++i) {
                const char ch = s[i];
                if (!inSingle && ch == '"' && (i == 0 || s[i - 1] != '\\')) inDouble = !inDouble;
                else if (!inDouble && ch == '\'') inSingle = !inSingle;
                else if (!inDouble && !inSingle && ch == '#') return s.substr(0, i);
            }
            return s;
        };

        auto unquote = [&](std::string_view v) -> std::string {
            v = trim(v);
            if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
                return std::string(v.substr(1, v.size() - 2));
            }
            return std::string(v);
        };

        auto fullKeyFromStack = [&](const std::vector<Frame>& frames) -> std::string {
            std::string fullKey;
            for (const auto& f : frames) {
                if (!fullKey.empty()) fullKey.push_back('.');
                fullKey += f.key;
            }
            return fullKey;
        };

        std::string line;
        while (std::getline(in, line)) {
            // Count indentation (spaces only).
            std::size_t indent = 0;
            while (indent < line.size() && line[indent] == ' ') ++indent;
            std::string_view content(line);
            content = content.substr(indent);
            content = trim(stripComment(content));
            if (content.empty()) continue;

            // Adjust nesting stack based on indent.
            while (!stack.empty() && indent <= stack.back().indent) stack.pop_back();

            // Sequence item: treat as multi-value for current stack path.
            if (content.rfind("-", 0) == 0) {
                if (stack.empty()) continue;
                auto item = trim(content.substr(1));
                const auto keyPath = fullKeyFromStack(stack);
                if (!keyPath.empty()) out.multi[keyPath].push_back(unquote(item));
                continue;
            }

            const auto colon = content.find(':');
            if (colon == std::string_view::npos) continue;

            const auto key = trim(content.substr(0, colon));
            if (key.empty()) continue;

            auto value = trim(content.substr(colon + 1));
            const bool hasValue = !value.empty();

            std::string fullKey;
            for (const auto& f : stack) {
                if (!fullKey.empty()) fullKey.push_back('.');
                fullKey += f.key;
            }
            if (!fullKey.empty()) fullKey.push_back('.');
            fullKey += std::string(key);

            if (hasValue) {
                out.scalar[fullKey] = unquote(value);
            } else {
                // New mapping section.
                stack.push_back(Frame{indent, std::string(key)});
            }
        }

        return out;
    };

    // Determine config file path (flag takes precedence over fixed path).
    std::string configFlag;
    std::string configPathDefault;
    for (auto* c = this; c; c = c->parent_) {
        if (configFlag.empty() && !c->configFileFlag_.empty()) configFlag = c->configFileFlag_;
        if (configPathDefault.empty() && !c->configFilePath_.empty()) configPathDefault = c->configFilePath_;
    }

    std::string configPath;
    if (!configFlag.empty()) configPath = parser.getFlag<std::string>(configFlag, "");
    if (configPath.empty()) configPath = configPathDefault;

    if (!configPath.empty()) {
        std::ifstream in(configPath);
        if (!in.is_open()) return std::string("failed to open config file: ") + configPath;

        FlatRaw raw;
        if (endsWith(configPath, ".json")) {
            if (auto parsed = parseJsonFlatten(in)) {
                raw = std::move(*parsed);
            } else {
                return std::string("failed to parse json config file: ") + configPath;
            }
        } else if (endsWith(configPath, ".yaml") || endsWith(configPath, ".yml")) {
            if (auto parsed = parseYamlFlatten(in)) {
                raw = std::move(*parsed);
            } else {
                return std::string("failed to parse yaml config file: ") + configPath;
            }
        } else if (endsWith(configPath, ".toml")) {
            if (auto parsed = parseTomlFlatten(in)) {
                raw = std::move(*parsed);
            } else {
                return std::string("failed to parse toml config file: ") + configPath;
            }
        } else {
            raw.scalar = parseEnvLike(in);
        }

        std::unordered_map<std::string, std::string> keyToLong;
        for (const auto& f : effectiveFlags()) {
            if (f.longName().empty()) continue;
            keyToLong[f.longName()] = f.longName();
            if (f.longName().rfind("--", 0) == 0) keyToLong[f.longName().substr(2)] = f.longName();
            if (f.longName().rfind("--", 0) == 0) {
                const auto base = f.longName().substr(2);
                std::string underscore = base;
                std::replace(underscore.begin(), underscore.end(), '-', '_');
                keyToLong[underscore] = f.longName();
                std::string dotted = base;
                std::replace(dotted.begin(), dotted.end(), '-', '.');
                keyToLong[dotted] = f.longName();
            }
            if (!f.varName().empty()) keyToLong[f.varName()] = f.longName();
        }

        for (const auto& [k, v] : raw.scalar) {
            const auto it = keyToLong.find(k);
            if (it == keyToLong.end()) continue;
            external[it->second] = v;
        }

        for (const auto& [k, vec] : raw.multi) {
            const auto it = keyToLong.find(k);
            if (it == keyToLong.end()) continue;
            if (vec.empty()) continue;
            external.erase(it->second);
            externalMulti[it->second] = vec;
        }
    }

    // Env overrides config.
    for (const auto& [flag, env] : effectiveEnvBindings()) {
        if (env.empty()) continue;
        if (const char* v = std::getenv(env.c_str())) {
            if (*v == '\0') continue;
            external[flag] = v;
            externalMulti.erase(flag);
        }
    }

    if (!externalMulti.empty()) {
        if (auto err = parser.setExternalValuesMultiChecked(std::move(externalMulti))) return err;
    }
    if (!external.empty()) {
        if (auto err = parser.setExternalValuesChecked(std::move(external))) return err;
    }
    return std::nullopt;
}

inline void Command::printCompletionFish(std::ostream& os) const {
    const auto rootName = name_;
    os << "# fish completion for " << rootName << "\n";

    const auto cfg = resolvedCompletionConfig();
    const bool wantCompleteCmds = cfg.addCompleteCommands;
    const bool wantCompleteDesc = wantCompleteCmds && !cfg.completeCommandName.empty();
    const bool wantCompleteNoDesc = wantCompleteCmds && !cfg.completeNoDescCommandName.empty();

    if (wantCompleteDesc || wantCompleteNoDesc) {
        const auto completeCmd = wantCompleteDesc ? cfg.completeCommandName : cfg.completeNoDescCommandName;
        const auto fnName = "__clasp_" + sanitizeIdentifier(rootName) + "_fish_complete";

        os << "function " << fnName << "\n";
        os << "  set -l words (commandline -opc)\n";
        os << "  if test (count $words) -gt 0\n";
        os << "    set -e words[1]\n";
        os << "  end\n";
        os << "  set -l cur (commandline -ct)\n";
        os << "  set -l prefix \"\"\n";
        os << "  if string match -rq '^-.+=.*' -- $cur\n";
        os << "    set -l parts (string split -m1 '=' -- $cur)\n";
        os << "    set -l eqflag $parts[1]\n";
        os << "    set cur $parts[2]\n";
        os << "    set -a words $eqflag\n";
        os << "    set prefix \"$eqflag=\"\n";
        os << "  end\n";
        os << "  set -l out (" << rootName << " " << completeCmd << " $words $cur 2>/dev/null)\n";
        os << "  set -l directive 0\n";
        os << "  set -l lines\n";
        os << "  for line in $out\n";
        os << "    if string match -rq '^:[0-9]+$' -- $line\n";
        os << "      set directive (string sub -s 2 -- $line)\n";
        os << "      continue\n";
        os << "    end\n";
        os << "    set -a lines $line\n";
        os << "  end\n";
        os << "  if test -z \"$directive\"\n";
        os << "    set directive 0\n";
        os << "  end\n";
        os << "  # ShellCompDirectiveError (1)\n";
        os << "  if test (math \"$directive & 1\") -ne 0\n";
        os << "    return 1\n";
        os << "  end\n";
        os << "  # ShellCompDirectiveFilterDirs (16)\n";
        os << "  if test (math \"$directive & 16\") -ne 0\n";
        os << "    for d in (__fish_complete_directories $cur)\n";
        os << "      echo $prefix$d\n";
        os << "    end\n";
        os << "    return 0\n";
        os << "  end\n";
        os << "  # ShellCompDirectiveFilterFileExt (8)\n";
        os << "  if test (math \"$directive & 8\") -ne 0\n";
        os << "    set -l dir '.'\n";
        os << "    set -l base $cur\n";
        os << "    if string match -rq '.+/.+' -- $cur\n";
        os << "      set -l p (string split -r -m1 '/' -- $cur)\n";
        os << "      if test (count $p) -ge 2\n";
        os << "        if test -n \"$p[1]\"\n";
        os << "          set dir $p[1]\n";
        os << "        end\n";
        os << "        set base $p[2]\n";
        os << "      end\n";
        os << "    end\n";
        os << "    set -l exts\n";
        os << "    for ext in $lines\n";
        os << "      set ext (string trim -l -c '.' -- $ext)\n";
        os << "      if test -n \"$ext\"\n";
        os << "        set -a exts $ext\n";
        os << "      end\n";
        os << "    end\n";
        os << "    set -l seen\n";
        os << "    for f in (command ls -1 $dir 2>/dev/null)\n";
        os << "      if not string match -q -- \"$base*\" $f\n";
        os << "        continue\n";
        os << "      end\n";
        os << "      if test -f \"$dir/$f\"\n";
        os << "        if test (count $exts) -eq 0\n";
        os << "          if not contains -- $f $seen\n";
        os << "            set -a seen $f\n";
        os << "            if test \"$dir\" = '.'\n";
        os << "              echo $prefix$f\n";
        os << "            else\n";
        os << "              echo $prefix$dir/$f\n";
        os << "            end\n";
        os << "          end\n";
        os << "          continue\n";
        os << "        end\n";
        os << "        for ext in $exts\n";
        os << "          if string match -rq \"\\\\.$ext$\" -- $f\n";
        os << "            if not contains -- $f $seen\n";
        os << "              set -a seen $f\n";
        os << "              if test \"$dir\" = '.'\n";
        os << "                echo $prefix$f\n";
        os << "              else\n";
        os << "                echo $prefix$dir/$f\n";
        os << "              end\n";
        os << "            end\n";
        os << "            break\n";
        os << "          end\n";
        os << "        end\n";
        os << "      end\n";
        os << "    end\n";
        os << "    return 0\n";
        os << "  end\n";
        os << "  for line in $lines\n";
        os << "    echo $prefix$line\n";
        os << "  end\n";
        os << "end\n";
        // `-k` keeps the order of suggestions (fish sorts by default). Cobra's KeepOrder directive maps best to this.
        os << "complete -c " << rootName << " -f -k -a '(" << fnName << ")'\n";
        return;
    }

    std::vector<std::string> rootSubs;
    for (const auto* sub : listVisibleSubcommands()) {
        rootSubs.push_back(sub->name_);
        for (const auto& a : sub->aliases_) rootSubs.push_back(a);
    }
    if (resolvedAddHelpCommand()) rootSubs.push_back(resolvedHelpCommandName());
    if (!resolvedVersion().empty()) rootSubs.push_back("version");

    os << "complete -c " << rootName << " -f -a \"" << joinWords(rootSubs) << "\"\n";

    // Global flags: include effective flags on root + built-ins.
    const auto rootFlags = effectiveFlags();
    for (const auto& f : rootFlags) {
        os << "complete -c " << rootName;
        if (!f.longName().empty()) os << " -l " << f.longName().substr(2);
        if (!f.shortName().empty()) os << " -s " << f.shortName().substr(1);
        if (!f.description().empty()) os << " -d \"" << escapeDoubleQuotes(f.description()) << "\"";
        os << "\n";
    }
    os << "complete -c " << rootName << " -l help -s h -d \"Help for this command\"\n";
    if (!resolvedVersion().empty()) os << "complete -c " << rootName << " -l version -d \"Version for this command\"\n";

    // Local flags: enable when the subcommand token appears.
    for (const auto* sub : listVisibleSubcommands()) {
        const auto inv = sub->invocationNames();
        const auto subFlags = sub->effectiveFlags();
        for (const auto& f : subFlags) {
            if (f.longName().empty() && f.shortName().empty()) continue;
            os << "complete -c " << rootName << " -n \"__fish_seen_subcommand_from " << joinWords(inv) << "\"";
            if (!f.longName().empty()) os << " -l " << f.longName().substr(2);
            if (!f.shortName().empty()) os << " -s " << f.shortName().substr(1);
            if (!f.description().empty()) os << " -d \"" << escapeDoubleQuotes(f.description()) << "\"";
            os << "\n";
        }
    }
}

inline void Command::printCompletionPowerShell(std::ostream& os) const {
    const auto rootName = name_;
    const auto cfg = resolvedCompletionConfig();
    const bool dynamicOk = cfg.addCompleteCommands && !cfg.completeNoDescCommandName.empty();

    if (dynamicOk) {
        os << "# PowerShell completion for " << rootName << "\n";
        os << "$__claspCompleteCmd = " << joinQuotedPowerShell({cfg.completeNoDescCommandName}) << "\n";

        os << "Register-ArgumentCompleter -CommandName " << rootName << " -ScriptBlock {\n";
        os << "  param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)\n";
        os << "  $origWordToComplete = $wordToComplete\n";
        os << "  $tokens = @(); foreach ($e in $commandAst.CommandElements) { $tokens += $e.ToString() }\n";
        os << "  $words = @(); if ($tokens.Count -gt 1) { $words = $tokens[1..($tokens.Count-1)] }\n";
        os << "  if ($words.Count -gt 0 -and $words[$words.Count-1] -eq $wordToComplete) { $words = $words[0..($words.Count-2)] }\n";
        os << "  $prefix = ''\n";
        os << "  if ($wordToComplete -match '^-.*=.*') {\n";
        os << "    $eq = $wordToComplete.IndexOf('=')\n";
        os << "    if ($eq -ge 0) {\n";
        os << "      $flag = $wordToComplete.Substring(0, $eq)\n";
        os << "      $wordToComplete = $wordToComplete.Substring($eq + 1)\n";
        os << "      $words += $flag\n";
        os << "      $prefix = $flag + '='\n";
        os << "    }\n";
        os << "  }\n";
        os << "  $raw = & $commandName $__claspCompleteCmd @words $wordToComplete 2>$null\n";
        os << "  if (-not $raw) { return }\n";
        os << "  $lines = @($raw)\n";
        os << "  $directive = 0\n";
        os << "  $last = $lines[$lines.Count-1]\n";
        os << "  if ($last -match '^:(\\d+)$') { $directive = [int]$matches[1]; if ($lines.Count -gt 1) { $lines = $lines[0..($lines.Count-2)] } else { $lines = @() } }\n";

        os << "  $cands = @()\n";
        os << "  if (($directive -band 16) -ne 0) {\n";
        os << "    $dir = Split-Path -Path $wordToComplete -Parent\n";
        os << "    if (-not $dir) { $dir = '.' }\n";
        os << "    $base = Split-Path -Path $wordToComplete -Leaf\n";
        os << "    $items = Get-ChildItem -Directory -Name -Path $dir -ErrorAction SilentlyContinue\n";
        os << "    foreach ($d in $items) {\n";
        os << "      if ($d -like \"$base*\") {\n";
        os << "        if ($dir -eq '.' -or $dir -eq '') { $cands += $d } else { $cands += (Join-Path $dir $d) }\n";
        os << "      }\n";
        os << "    }\n";
        os << "  } elseif (($directive -band 8) -ne 0) {\n";
        os << "    $dir = Split-Path -Path $wordToComplete -Parent\n";
        os << "    if (-not $dir) { $dir = '.' }\n";
        os << "    $base = Split-Path -Path $wordToComplete -Leaf\n";
        os << "    $exts = @(); foreach ($e in $lines) { if ($e) { $exts += $e.TrimStart('.') } }\n";
        os << "    $items = Get-ChildItem -File -Name -Path $dir -ErrorAction SilentlyContinue\n";
        os << "    foreach ($f in $items) {\n";
        os << "      if ($f -notlike \"$base*\") { continue }\n";
        os << "      $ext = [System.IO.Path]::GetExtension($f).TrimStart('.')\n";
        os << "      if ($exts.Count -eq 0 -or ($exts -contains $ext)) {\n";
        os << "        if ($dir -eq '.' -or $dir -eq '') { $cands += $f } else { $cands += (Join-Path $dir $f) }\n";
        os << "      }\n";
        os << "    }\n";
        os << "  } else {\n";
        os << "    $cands = $lines\n";
        os << "  }\n";

        os << "  if ($prefix) { $cands = $cands | ForEach-Object { $prefix + $_ } }\n";
        os << "  if (($directive -band 1) -ne 0) { return }\n";
        os << "  $cands | Where-Object { $_ -like \"$origWordToComplete*\" } | ForEach-Object {\n";
        os << "    [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n";
        os << "  }\n";
        os << "}\n";
        return;
    }

    const auto entries = completionEntries();

    struct FlatEntry {
        std::string path;
        std::vector<std::string> subcommands;
        std::vector<std::string> flags;
    };
    std::vector<FlatEntry> flat;
    for (const auto& e : entries) {
        for (const auto& p : e.pathAlternatives) {
            flat.push_back({p, e.subcommands, e.flags});
        }
    }

    os << "# PowerShell completion for " << rootName << "\n";
    os << "$__claspSubs = @{}\n";
    os << "$__claspFlags = @{}\n";
    for (const auto& e : flat) {
        os << "$__claspSubs[" << joinQuotedPowerShell({e.path}) << "] = @(" << joinQuotedPowerShell(e.subcommands) << ")\n";
        os << "$__claspFlags[" << joinQuotedPowerShell({e.path}) << "] = @(" << joinQuotedPowerShell(e.flags) << ")\n";
    }

    os << "Register-ArgumentCompleter -CommandName " << rootName << " -ScriptBlock {\n";
    os << "  param($commandName, $parameterName, $wordToComplete, $commandAst, $fakeBoundParameters)\n";
    os << "  $tokens = @(); foreach ($e in $commandAst.CommandElements) { $tokens += $e.ToString() }\n";
    os << "  $cmd = \"" << rootName << "\"\n";
    os << "  for ($i = 1; $i -lt $tokens.Count; $i++) {\n";
    os << "    $t = $tokens[$i]\n";
    os << "    if ($t -like \"-*\") { continue }\n";
    os << "    $subs = $__claspSubs[$cmd]\n";
    os << "    if ($subs -and ($subs -contains $t)) { $cmd = $cmd + \" \" + $t } else { break }\n";
    os << "  }\n";
    os << "  $subs = $__claspSubs[$cmd]; $flags = $__claspFlags[$cmd]\n";
    os << "  if (-not $subs) { $subs = @() }\n";
    os << "  if (-not $flags) { $flags = @() }\n";
    os << "  $cands = if ($wordToComplete -like \"-*\") { $flags } else { $subs }\n";
    os << "  $cands | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object {\n";
    os << "    [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n";
    os << "  }\n";
    os << "}\n";
}

inline Command& Command::enableCompletion() { return enableCompletion(CompletionConfig{}); }

inline Command& Command::enableCompletion(CompletionConfig cfg) {
    if (!isRoot()) return *this;
    completionConfigOverride_ = cfg;
    const auto hasNamedSubcommand = [&](const std::string& name) {
        for (const auto& c : subcommands_) {
            if (c->name_ == name) return true;
        }
        return false;
    };

    const bool wantCompletionCmd = cfg.addCompletionCommand && !cfg.completionCommandName.empty();
    const bool wantCompleteCmds = cfg.addCompleteCommands;
    const bool wantCompleteDesc = wantCompleteCmds && !cfg.completeCommandName.empty();
    const bool wantCompleteNoDesc = wantCompleteCmds && !cfg.completeNoDescCommandName.empty();

    if (wantCompletionCmd && !hasNamedSubcommand(cfg.completionCommandName)) {
        Command completionCmd(cfg.completionCommandName, "Generate shell completion scripts");
        completionCmd.args([](const std::vector<std::string>& args) -> std::optional<std::string> {
            if (args.size() == 1) return std::nullopt;
            return std::string("accepts 1 arg(s)");
        });
        completionCmd.action([this](Command& cmd, const Parser&, const std::vector<std::string>& args) {
            const auto& shell = args[0];
            if (shell == "bash") {
                this->printCompletionBash(cmd.outOrStdout());
                return 0;
            }
            if (shell == "zsh") {
                this->printCompletionZsh(cmd.outOrStdout());
                return 0;
            }
            if (shell == "fish") {
                this->printCompletionFish(cmd.outOrStdout());
                return 0;
            }
            if (shell == "powershell") {
                this->printCompletionPowerShell(cmd.outOrStdout());
                return 0;
            }
            cmd.errOrStderr() << "unknown shell: " << shell << "\n";
            cmd.errOrStderr() << "supported shells: bash, zsh, fish, powershell\n";
            return 1;
        });
        addCommand(std::move(completionCmd));
    }

    auto addComplete = [&](std::string name, bool withDescriptions) {
        Command completeCmd(std::move(name), "Internal completion command");
        completeCmd.hidden(true);
        completeCmd.disableFlagParsing(true);
        completeCmd.action([this, withDescriptions](Command& cmd, const Parser&, const std::vector<std::string>& args) {
            std::vector<std::string> words;
            std::string toComplete;
            if (!args.empty()) {
                words.assign(args.begin(), args.end() - 1);
                toComplete = args.back();
            }

            const auto items = this->completeWords(words, toComplete, withDescriptions);
            for (const auto& it : items) {
                if (!it.description.empty()) cmd.outOrStdout() << it.value << "\t" << it.description << "\n";
                else cmd.outOrStdout() << it.value << "\n";
            }
            return 0;
        });
        addCommand(std::move(completeCmd));
    };

    if (wantCompleteDesc && !hasNamedSubcommand(cfg.completeCommandName)) {
        addComplete(cfg.completeCommandName, /*withDescriptions=*/true);
    }
    if (wantCompleteNoDesc && !hasNamedSubcommand(cfg.completeNoDescCommandName)) {
        addComplete(cfg.completeNoDescCommandName, /*withDescriptions=*/false);
    }
    return *this;
}

// Args validation helpers (cobra-like).
inline Command::ArgsValidator NoArgs() {
    return [](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.empty()) return std::nullopt;
        return std::string("accepts no arguments");
    };
}

inline Command::ArgsValidator ExactArgs(std::size_t n) {
    return [n](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() == n) return std::nullopt;
        return std::string("accepts ") + std::to_string(n) + " arg(s)";
    };
}

inline Command::ArgsValidator MinimumNArgs(std::size_t n) {
    return [n](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() >= n) return std::nullopt;
        return std::string("requires at least ") + std::to_string(n) + " arg(s)";
    };
}

inline Command::ArgsValidator MaximumNArgs(std::size_t n) {
    return [n](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() <= n) return std::nullopt;
        return std::string("accepts at most ") + std::to_string(n) + " arg(s)";
    };
}

inline Command::ArgsValidator RangeArgs(std::size_t minN, std::size_t maxN) {
    return [minN, maxN](const std::vector<std::string>& args) -> std::optional<std::string> {
        if (args.size() >= minN && args.size() <= maxN) return std::nullopt;
        return std::string("accepts between ") + std::to_string(minN) + " and " + std::to_string(maxN) + " arg(s)";
    };
}

} // namespace clasp

#endif // CLASP_COMMAND_HPP
