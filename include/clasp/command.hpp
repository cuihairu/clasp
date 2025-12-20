#ifndef CLASP_COMMAND_HPP
#define CLASP_COMMAND_HPP

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "flag.hpp"
#include "parser.hpp"
#include "utils.hpp"

namespace clasp {

class Command {
public:
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

    Command& markPersistentFlagDeprecated(const std::string& name, std::string msg) {
        if (auto* f = findFlagMutable(persistentFlags_, normalizeFlagName(name))) f->setDeprecated(std::move(msg));
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

    Command& hidden(bool v = true) {
        hidden_ = v;
        return *this;
    }

    Command& deprecated(std::string msg) {
        deprecated_ = std::move(msg);
        return *this;
    }

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

    int run(int argc, char** argv) {
        auto resolution = resolveForExecution(argc, argv);
        if (resolution.helpRequested) {
            return execHelp(*resolution.helpBase, resolution.helpPath);
        }
        if (resolution.versionRequested) {
            const auto v = resolution.cmd->resolvedVersion();
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
        parseOpts.suggestFlags = resolution.cmd->resolvedSuggestions();
        parseOpts.suggestionsMinimumDistance = resolution.cmd->resolvedSuggestionsMinimumDistance();
        Parser parser(static_cast<int>(argvVec.size()), argvPtrs.data(), effectiveFlags, parseOpts);
        if (!parser.ok()) {
            return resolution.cmd->fail(parser.error(), /*showUsage=*/true);
        }

        if (parser.hasFlag("--help") || parser.hasFlag("-h")) {
            resolution.cmd->printHelp();
            return 0;
        }

        const auto& positionals = parser.positionals();

        // `help` and `version` subcommands are treated as root-only, cobra-like.
        if (resolution.cmd == this && !positionals.empty() && positionals.front() == "help") {
            std::vector<std::string> path(positionals.begin() + 1, positionals.end());
            return execHelp(*this, path);
        }
        if (resolution.cmd == this && !positionals.empty() && positionals.front() == "version") {
            const auto v = resolvedVersion();
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

        // Version flag is global.
        if (parser.hasFlag("--version")) {
            const auto v = resolution.cmd->resolvedVersion();
            if (!v.empty()) resolution.cmd->out() << v << "\n";
            return 0;
        }

        if (resolution.cmd->args_) {
            if (auto err = resolution.cmd->args_(positionals)) {
                return resolution.cmd->fail(*err, /*showUsage=*/true);
            }
        }

        if (auto err = resolution.cmd->checkRequiredFlags(parser)) {
            return resolution.cmd->fail(*err, /*showUsage=*/true);
        }

        if (auto err = resolution.cmd->checkFlagGroups(parser)) {
            return resolution.cmd->fail(*err, /*showUsage=*/true);
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
        printUsageTo(out());

        const auto cmdShort = short_.empty() ? long_ : short_;
        if (!cmdShort.empty()) out() << "\n" << cmdShort << "\n";

        const auto visibleSubcommands = listVisibleSubcommands();
        const bool showCommands = !visibleSubcommands.empty() || (isRoot() && suggestions_);
        if (showCommands) {
            out() << "\nCommands:\n";
            if (isRoot() && suggestions_) out() << "  help - Help about any command\n";
            if (isRoot() && suggestions_ && !resolvedVersion().empty()) out() << "  version - Print the version number\n";
            for (const auto* sub : visibleSubcommands) {
                out() << "  " << sub->name_ << " - " << sub->short_ << "\n";
            }
        }

        const auto [localFlags, globalFlags] = flagsForHelp();
        if (!localFlags.empty()) {
            out() << "\nFlags:\n";
            for (const auto* f : localFlags) out() << "  " << formatFlag(*f) << "\n";
        }
        if (!globalFlags.empty()) {
            out() << "\nGlobal Flags:\n";
            for (const auto* f : globalFlags) out() << "  " << formatFlag(*f) << "\n";
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

    Command& enableCompletion();

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

    bool resolvedSuggestions() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->suggestionsOverride_.has_value()) return *c->suggestionsOverride_;
        }
        return true;
    }

    std::size_t resolvedSuggestionsMinimumDistance() const {
        for (auto* c = this; c; c = c->parent_) {
            if (c->suggestionsMinimumDistanceOverride_.has_value()) return *c->suggestionsMinimumDistanceOverride_;
        }
        return 2;
    }

    bool runnable() const { return static_cast<bool>(action_) || static_cast<bool>(actionE_); }

    void printUsageTo(std::ostream& os) const {
        os << "Usage: " << commandPath();
        if (!listVisibleSubcommands().empty() || (isRoot() && suggestions_)) {
            os << " [command]";
        }
        os << " [flags]\n";
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
        return out;
    }

    static std::string formatFlag(const Flag& f) {
        std::string names;
        if (!f.shortName().empty()) names += f.shortName() + ", ";
        names += f.longName();

        std::string desc = f.description();
        if (!f.deprecated().empty()) desc += " (deprecated: " + f.deprecated() + ")";
        if (f.required()) desc += " (required)";
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
            err() << "Error: " << message << "\n";
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
        if (!silenceErrors_) err() << "Error: " << msg << "\n";
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
            const auto flags = at->effectiveFlags();
            for (const auto& f : flags) {
                if (f.longName() == key || f.shortName() == key) {
                    return FlagInfo{std::holds_alternative<bool>(f.defaultValue())};
                }
            }
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            return std::nullopt;
        };

        auto flagInfoSubtree = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};

            bool found = false;
            bool anyNonBool = false;

            std::function<void(const Command*)> visit = [&](const Command* c) {
                const auto flags = c->effectiveFlags();
                for (const auto& f : flags) {
                    if (f.longName() == key || f.shortName() == key) {
                        found = true;
                        if (!std::holds_alternative<bool>(f.defaultValue())) anyNonBool = true;
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

            if (token.rfind("--no-", 0) == 0) return;

            if (isShortGroupToken(token)) {
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
        const std::string tok(token);
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
        opts.suggestFlags = cmd->resolvedSuggestions();
        opts.suggestionsMinimumDistance = cmd->resolvedSuggestionsMinimumDistance();
        const Parser parser(static_cast<int>(argvVec.size()), argvPtrs.data(), cmd->effectiveFlags(), opts);
        const auto& args = parser.positionals();

        // Value completion for --flag=prefix
        if (const auto eq = toCompleteStr.find('='); eq != std::string::npos) {
            const auto keyPart = toCompleteStr.substr(0, eq);
            const auto valPrefix = toCompleteStr.substr(eq + 1);
            if (auto longName = resolveFlagNameForValueCompletion(*cmd, keyPart)) {
                if (auto* func = findFlagCompletionFunc(cmd, *longName)) {
                    for (auto& cand : (*func)(*cmd, parser, args, valPrefix)) {
                        if (cand.rfind(valPrefix, 0) != 0) continue;
                        out.push_back({keyPart + "=" + cand, {}});
                    }
                }
            }
            out.push_back({":4", {}});
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
                            for (auto& cand : (*func)(*cmd, parser, args, toComplete)) {
                                if (cand.rfind(toCompleteStr, 0) != 0) continue;
                                out.push_back({cand, {}});
                            }
                        }
                        out.push_back({":4", {}});
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
            out.push_back({":4", {}});
            return out;
        }

        // Subcommands.
        {
            std::set<std::string> seen;
            for (const auto* sub : cmd->listVisibleSubcommands()) {
                for (const auto& name : sub->invocationNames()) {
                    if (name.rfind(toCompleteStr, 0) != 0) continue;
                    if (!seen.insert(name).second) continue;
                    out.push_back({name, withDescriptions ? sub->short_ : ""});
                }
            }
            if (cmd->isRoot()) {
                if (std::string("help").rfind(toCompleteStr, 0) == 0) {
                    out.push_back({"help", withDescriptions ? "Help about any command" : ""});
                }
                if (!cmd->resolvedVersion().empty() && std::string("version").rfind(toCompleteStr, 0) == 0) {
                    out.push_back({"version", withDescriptions ? "Print the version number" : ""});
                }
            }
        }

        // Valid args / dynamic args completion.
        if (cmd->validArgsFunction_) {
            for (auto& cand : cmd->validArgsFunction_(*cmd, parser, args, toComplete)) {
                if (cand.rfind(toCompleteStr, 0) != 0) continue;
                out.push_back({cand, {}});
            }
        } else {
            for (const auto& cand : cmd->validArgs_) {
                if (cand.rfind(toCompleteStr, 0) != 0) continue;
                out.push_back({cand, {}});
            }
        }

        out.push_back({":4", {}});
        return out;
    }

    std::unordered_map<std::string, std::string> effectiveEnvBindings() const;
    std::optional<std::string> applyExternalSources(Parser& parser) const;

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

        std::set<int> commandTokenIdx;
        bool positionalOnly = false;

        struct FlagInfo {
            bool isBool{false};
        };

        auto flagInfoLocal = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            const auto flags = at->effectiveFlags();
            for (const auto& f : flags) {
                if (f.longName() == key || f.shortName() == key) {
                    return FlagInfo{std::holds_alternative<bool>(f.defaultValue())};
                }
            }
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            return std::nullopt;
        };

        auto flagInfoSubtree = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};

            bool found = false;
            bool anyNonBool = false;

            std::function<void(const Command*)> visit = [&](const Command* c) {
                const auto flags = c->effectiveFlags();
                for (const auto& f : flags) {
                    if (f.longName() == key || f.shortName() == key) {
                        found = true;
                        if (!std::holds_alternative<bool>(f.defaultValue())) anyNonBool = true;
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

            if (token.rfind("--no-", 0) == 0) return;

            if (token.size() >= 3 && token[0] == '-' && token[1] != '-') {
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

            if (token == "help" && r.cmd == this) {
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
    bool silenceUsage_{false};
    bool silenceErrors_{false};
    bool suggestions_{true};
    std::size_t suggestionsMinimumDistance_{2};
    std::optional<bool> suggestionsOverride_;
    std::optional<std::size_t> suggestionsMinimumDistanceOverride_;
    bool disableFlagParsing_{false};
    bool traverseChildren_{false};
    std::vector<std::vector<std::string>> mutuallyExclusiveFlagGroups_;
    std::vector<std::vector<std::string>> oneRequiredFlagGroups_;
    std::vector<std::vector<std::string>> requiredTogetherFlagGroups_;
    std::vector<std::string> validArgs_;
    CompletionFunc validArgsFunction_;
    std::unordered_map<std::string, CompletionFunc> flagCompletionFuncs_;
    std::unordered_map<std::string, std::string> envBindings_;
    std::string configFilePath_;
    std::string configFileFlag_;
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

    CompletionEntry helpEntry;
    helpEntry.primaryPath = name_ + " help";
    helpEntry.pathAlternatives = {helpEntry.primaryPath};
    for (const auto* sub : listVisibleSubcommands()) {
        helpEntry.subcommands.push_back(sub->name_);
        for (const auto& a : sub->aliases_) helpEntry.subcommands.push_back(a);
    }
    helpEntry.flags = {"--help", "-h"};
    if (!resolvedVersion().empty()) helpEntry.flags.push_back("--version");
    entries.push_back(std::move(helpEntry));

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
        e.subcommands.push_back("help");
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
    const auto entries = completionEntries();

    if (forZsh) os << "#compdef " << rootName << "\n\nautoload -U +X bashcompinit && bashcompinit\n\n";
    os << "# bash completion for " << rootName << "\n";
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

inline std::optional<std::string> Command::applyExternalSources(Parser& parser) const {
    std::unordered_map<std::string, std::string> external;

    auto trim = [](std::string_view s) -> std::string_view {
        std::size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        std::size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
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

        std::unordered_map<std::string, std::string> keyToLong;
        for (const auto& f : effectiveFlags()) {
            if (f.longName().empty()) continue;
            keyToLong[f.longName()] = f.longName();
            if (f.longName().rfind("--", 0) == 0) keyToLong[f.longName().substr(2)] = f.longName();
            if (!f.varName().empty()) keyToLong[f.varName()] = f.longName();
        }

        for (const auto& [k, v] : raw) {
            const auto it = keyToLong.find(k);
            if (it == keyToLong.end()) continue;
            external[it->second] = v;
        }
    }

    // Env overrides config.
    for (const auto& [flag, env] : effectiveEnvBindings()) {
        if (env.empty()) continue;
        if (const char* v = std::getenv(env.c_str())) {
            if (*v == '\0') continue;
            external[flag] = v;
        }
    }

    if (!external.empty()) parser.setExternalValues(std::move(external));
    return std::nullopt;
}

inline void Command::printCompletionFish(std::ostream& os) const {
    const auto rootName = name_;
    os << "# fish completion for " << rootName << "\n";

    std::vector<std::string> rootSubs;
    for (const auto* sub : listVisibleSubcommands()) {
        rootSubs.push_back(sub->name_);
        for (const auto& a : sub->aliases_) rootSubs.push_back(a);
    }
    rootSubs.push_back("help");
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

inline Command& Command::enableCompletion() {
    if (!isRoot()) return *this;
    bool hasCompletionCmd = false;
    bool hasCompleteCmd = false;
    bool hasCompleteNoDescCmd = false;
    for (const auto& c : subcommands_) {
        if (c->name_ == "completion") hasCompletionCmd = true;
        if (c->name_ == "__complete") hasCompleteCmd = true;
        if (c->name_ == "__completeNoDesc") hasCompleteNoDescCmd = true;
    }

    if (!hasCompletionCmd) {
        Command completionCmd("completion", "Generate shell completion scripts");
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

    if (!hasCompleteCmd) addComplete("__complete", /*withDescriptions=*/true);
    if (!hasCompleteNoDescCmd) addComplete("__completeNoDesc", /*withDescriptions=*/false);
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
