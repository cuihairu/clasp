#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "flag.hpp"


namespace clasp {
    class Parser {
    public:
        Parser(int argc, char** argv, const std::vector<Flag>& flags) {
            for (int i = 1; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg.rfind("--", 0) == 0 || arg.rfind("-", 0) == 0) {
                    std::string key = arg;
                    std::string value = (i + 1 < argc) ? argv[++i] : "";
                    flagValues_[key] = value;
                } else {
                    commands_.emplace_back(arg);
                }
            }
        }

        bool hasFlag(const std::string& flag) const {
            return flagValues_.find(flag) != flagValues_.end();
        }

        template <typename T>
        T getFlag(const std::string& flag) const {
            auto it = flagValues_.find(flag);
            if (it != flagValues_.end()) {
                return static_cast<T>(it->second);
            }
            return T();
        }

        bool hasCommand(const std::string& command) const {
            return std::find(commands_.begin(), commands_.end(), command) != commands_.end();
        }

    private:
        std::unordered_map<std::string, std::string> flagValues_;
        std::vector<std::string> commands_;
    };
}
#endif //PARSER_HPP
