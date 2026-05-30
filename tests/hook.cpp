#include <iostream>
#include <string>
#include <vector>

#include "clasp/clasp.hpp"

int main(int argc, char** argv) {
    clasp::Command root("hook", "Hook coverage");
    root.suggestions(false);

    // run: normal hook execution order — execute runCmd directly (not its sub)
    clasp::Command runCmd("run", "Run with hooks");
    runCmd.persistentPreRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ppre:";
    });
    runCmd.preRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "pre:";
    });
    runCmd.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "action:";
        return 0;
    });
    runCmd.postRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "post:";
    });
    runCmd.persistentPostRun([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "ppost\n";
    });

    // err-pre: preRunE error — execute errPre directly
    clasp::Command errPre("err-pre", "Pre-run error");
    errPre.preRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                       -> std::optional<std::string> {
        return "pre-run error";
    });
    errPre.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        std::cout << "should not reach\n";
        return 0;
    });

    // err-post: postRunE error — execute errPost directly
    clasp::Command errPost("err-post", "Post-run error");
    errPost.postRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                        -> std::optional<std::string> {
        return "post-run error";
    });
    errPost.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });

    // err-ppre: persistentPreRunE error via subcommand
    clasp::Command errPpre("err-ppre", "Persistent pre error");
    errPpre.persistentPreRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                                  -> std::optional<std::string> {
        return "persistent pre error";
    });
    clasp::Command errPpreSub("sub", "Err ppre sub");
    errPpreSub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });
    errPpre.addCommand(std::move(errPpreSub));

    // err-ppost: persistentPostRunE error via subcommand
    clasp::Command errPpost("err-ppost", "Persistent post error");
    errPpost.persistentPostRunE([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&)
                                    -> std::optional<std::string> {
        return "persistent post error";
    });
    clasp::Command errPpostSub("sub", "Err ppost sub");
    errPpostSub.action([](clasp::Command&, const clasp::Parser&, const std::vector<std::string>&) {
        return 0;
    });
    errPpost.addCommand(std::move(errPpostSub));

    root.addCommand(std::move(runCmd));
    root.addCommand(std::move(errPre));
    root.addCommand(std::move(errPost));
    root.addCommand(std::move(errPpre));
    root.addCommand(std::move(errPpost));

    return root.run(argc, argv);
}
