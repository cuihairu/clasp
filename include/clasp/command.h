#ifndef CLASP_COMMAND_HPP
#define CLASP_COMMAND_HPP

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <unordered_map>
#include "flag.hpp"
#include "parser.hpp"

namespace clasp {

class Command : public std::enable_shared_from_this<Command> {
public:
    using Action = std::function<int(std::shared_ptr<Command>& cmd, const std::vector<std::string>& flags)>;

    Command(std::string name,std::string intro, std::string description)
        : name_(std::move(name)),intro_(std::move(intro)), description_(std::move(description)),parent_command_() {}

    Command& addCommand(Command cmd) {
        subcommands_.emplace_back(std::move(cmd));
        subcommands_.back().setParentCommand(shared_from_this());
        return *this;
    }

    Command& withFlag(std::string longName, std::string shortName,std::string varName, std::string description, FlagValue defaultValue) {
        flags_.emplace_back(longName, shortName,varName, description, defaultValue);
        return *this;
    }

    Command& withFlag(std::string longName, std::string shortName, std::string description){
        flags_.emplace_back(longName, shortName, description);
        return *this;
    }


    Command& action(Action action) {
        action_ = std::move(action);
        return *this;
    }

    void run(int argc, char** argv) const {
        Parser parser(argc, argv, flags_);
        if (parser.hasFlag("--help") || parser.hasFlag("-h")) {
            printHelp();
            return;
        }

        if (action_) {
            action_(shared_from_this(),argv);
        }

        for (const auto& subcmd : subcommands_) {
            if (parser.hasCommand(subcmd.name_)) {
                subcmd.run(argc - 1, argv + 1);
            }
        }
    }

    void printHelp() const {
        std::cout << "Usage: " << name_ << " [command] [flags]\n\n";
        std::cout << description_ << "\n\n";
        std::cout << "Commands:\n";
        for (const auto& subcmd : subcommands_) {
            std::cout << "  " << subcmd.name_ << " - " << subcmd.description_ << "\n";
        }
        std::cout << "\nFlags:\n";
        for (const auto& flag : flags_) {
            std::cout << "  " << flag.longName() << ", " << flag.shortName() << " - " << flag.description() << "\n";
        }
    }
protected:
    void setParentCommand(std::shared_ptr<Command> parent) {
        parent_command_ = parent;
    }
private:
    std::string name_;
    std::string intro_;
    std::string description_;
    std::vector<Flag> flags_;
    std::vector<Command> subcommands_;
    Action action_;
    std::string version_;
    std::shared_ptr<Command> parent_command_;
};

} // namespace Clasp

#endif // CLASP_COMMAND_HPP
