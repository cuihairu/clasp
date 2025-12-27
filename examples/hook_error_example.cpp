#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("app", "Hook error example");
    root.withPersistentFlag("--root-pre-err", "", "root-pre-err", "Fail in persistentPreRunE", false);
    root.withPersistentFlag("--root-post-err", "", "root-post-err", "Fail in persistentPostRunE", false);

    root.persistentPreRunE([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        if (parser.getFlag<bool>("--root-pre-err", false)) return std::optional<std::string>("root-pre");
        return std::optional<std::string>();
    });
    root.persistentPostRunE([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        if (parser.getFlag<bool>("--root-post-err", false)) return std::optional<std::string>("root-post");
        return std::optional<std::string>();
    });

    clasp::Command doCmd("do", "Do work");
    doCmd.withFlag("--sub-pre-err", "", "sub-pre-err", "Fail in preRunE", false);
    doCmd.withFlag("--sub-post-err", "", "sub-post-err", "Fail in postRunE", false);
    doCmd.withFlag("--action-err", "", "action-err", "Fail in actionE", false);

    doCmd.preRunE([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        if (parser.getFlag<bool>("--sub-pre-err", false)) return std::optional<std::string>("sub-pre");
        return std::optional<std::string>();
    });

    doCmd.actionE([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        if (parser.getFlag<bool>("--action-err", false)) return std::optional<std::string>("action");
        return std::optional<std::string>();
    });

    doCmd.postRunE([](clasp::Command&, const clasp::Parser& parser, const std::vector<std::string>&) {
        if (parser.getFlag<bool>("--sub-post-err", false)) return std::optional<std::string>("sub-post");
        return std::optional<std::string>();
    });

    root.addCommand(std::move(doCmd));
    return root.run(argc, argv);
}

