#ifndef CLASP_COMMAND_HPP
#define CLASP_COMMAND_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
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

    explicit Command(std::string name, std::string shortDesc = {}, std::string longDesc = {})
        : name_(std::move(name)),
          short_(std::move(shortDesc)),
          long_(std::move(longDesc)) {}

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
            std::cout << v << "\n";
            return 0;
        }

        auto argvVec = std::move(resolution.argvForCmd);
        std::vector<char*> argvPtrs;
        argvPtrs.reserve(argvVec.size());
        for (auto& s : argvVec) argvPtrs.push_back(s.data());

        const auto effectiveFlags = resolution.cmd->effectiveFlags();
        Parser parser(static_cast<int>(argvVec.size()), argvPtrs.data(), effectiveFlags);
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
            if (!v.empty()) std::cout << v << "\n";
            return 0;
        }

        // Deprecation warnings (cobra-like: warn but continue).
        if (!resolution.cmd->deprecated_.empty()) {
            std::cerr << "Command \"" << resolution.cmd->commandPath() << "\" is deprecated: " << resolution.cmd->deprecated_
                      << "\n";
        }
        resolution.cmd->warnDeprecatedFlags(parser);

        // Version flag is global.
        if (parser.hasFlag("--version")) {
            const auto v = resolution.cmd->resolvedVersion();
            if (!v.empty()) std::cout << v << "\n";
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
        printUsageTo(std::cout);

        const auto cmdShort = short_.empty() ? long_ : short_;
        if (!cmdShort.empty()) std::cout << "\n" << cmdShort << "\n";

        const auto visibleSubcommands = listVisibleSubcommands();
        const bool showCommands = !visibleSubcommands.empty() || (isRoot() && suggestions_);
        if (showCommands) {
            std::cout << "\nCommands:\n";
            if (isRoot() && suggestions_) std::cout << "  help - Help about any command\n";
            if (isRoot() && suggestions_ && !resolvedVersion().empty()) std::cout << "  version - Print the version number\n";
            for (const auto* sub : visibleSubcommands) {
                std::cout << "  " << sub->name_ << " - " << sub->short_ << "\n";
            }
        }

        const auto [localFlags, globalFlags] = flagsForHelp();
        if (!localFlags.empty()) {
            std::cout << "\nFlags:\n";
            for (const auto* f : localFlags) std::cout << "  " << formatFlag(*f) << "\n";
        }
        if (!globalFlags.empty()) {
            std::cout << "\nGlobal Flags:\n";
            for (const auto* f : globalFlags) std::cout << "  " << formatFlag(*f) << "\n";
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
        }
        return nullptr;
    }

    const Command* findSubcommand(const std::string& name) const {
        for (const auto& c : subcommands_) {
            if (c->name_ == name) return c.get();
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

    bool runnable() const { return static_cast<bool>(action_) || static_cast<bool>(actionE_); }

    void printUsageTo(std::ostream& os) const {
        os << "Usage: " << commandPath();
        if (!listVisibleSubcommands().empty() || (isRoot() && suggestions_)) {
            os << " [command]";
        }
        os << " [flags]\n";
    }

    bool isRoot() const { return parent_ == nullptr; }

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
            if (parser.hasFlag(f.longName())) continue;
            return std::string("required flag not set: ") + f.longName();
        }
        return std::nullopt;
    }

    void warnDeprecatedFlags(const Parser& parser) const {
        const auto effective = effectiveFlags();
        for (const auto& f : effective) {
            if (f.deprecated().empty()) continue;
            if (!parser.hasFlag(f.longName())) continue;
            std::cerr << "Flag \"" << f.longName() << "\" is deprecated: " << f.deprecated() << "\n";
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
            std::cerr << "Error: " << message << "\n";
        }
        if (showUsage && !silenceUsage_) {
            std::cerr << "\n";
            printUsageTo(std::cerr);
        }
        return 1;
    }

    int failUnknownCommand(const std::string& token) const {
        std::string msg = "unknown command \"" + token + "\" for \"" + commandPath() + "\"";
        if (suggestions_) {
            std::vector<std::string> names;
            for (const auto* c : listVisibleSubcommands()) names.push_back(c->name_);
            const auto sugg = utils::suggest(token, names);
            if (!sugg.empty()) {
                msg += "\n\nDid you mean this?\n";
                for (const auto& s : sugg) msg += "  " + s + "\n";
            }
        }
        if (!silenceErrors_) std::cerr << "Error: " << msg << "\n";
        std::cerr << "Run '" << commandPath() << " --help' for usage.\n";
        return 1;
    }

    void reparent(Command* parent) {
        parent_ = parent;
        for (auto& c : subcommands_) c->reparent(this);
    }

    Resolution resolveForExecution(int argc, char** argv) {
        Resolution r;
        r.cmd = this;

        std::set<int> commandTokenIdx;
        bool positionalOnly = false;

        struct FlagInfo {
            bool isBool{false};
        };

        auto flagInfo = [&](Command* at, const std::string& key) -> std::optional<FlagInfo> {
            const auto flags = at->effectiveFlags();
            for (const auto& f : flags) {
                if (f.longName() == key || f.shortName() == key) {
                    return FlagInfo{std::holds_alternative<bool>(f.defaultValue())};
                }
            }
            if (key == "--help" || key == "-h" || key == "--version") return FlagInfo{true};
            return std::nullopt;
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
};

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
