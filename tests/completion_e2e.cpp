#include "completion_support.hpp"

void testShellEscapingEndToEnd() {
#ifndef _WIN32
    const bool haveBash = std::system("command -v bash >/dev/null 2>&1") == 0;
    const bool haveZsh = std::system("command -v zsh >/dev/null 2>&1") == 0;
    const bool haveFish = std::system("command -v fish >/dev/null 2>&1") == 0;
    if (!haveBash && !haveZsh && !haveFish) return;

    // A command tree whose names stress every shell-special character.
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompletionCommand = false;
    staticCfg.addCompleteCommands = false;

    clasp::Command root("e2eapp", "End-to-end root");
    root.enableCompletion(staticCfg);
    clasp::Command sub("don't", "Apostrophe sub");
    sub.addAlias("$x");
    // Command-substitution-shaped aliases must survive the -a and -n second
    // evaluation as literals, not be executed as fish code.
    sub.addAlias("`cmd`");
    sub.addAlias("$(cmdx)");
    sub.addAlias("(paren)");
    // A trailing backslash (odd count) is the poison token for fish single
    // quotes: unescaped, it escapes the closing quote on the second
    // evaluation and the generated script stops parsing.
    sub.addAlias("bs\\");
    // Even counts of backslashes (two, and two in the middle of a word) are
    // the silent regression: without per-backslash escaping, `bs\\` survives
    // the first evaluation as two backslashes but the second evaluation
    // collapses the pair to one (`\\` -> `\`), so the word is corrupted
    // without any parse error to catch it. Pin the round-trip here.
    sub.addAlias("bs\\\\");
    sub.addAlias("a\\\\b");
    sub.withFlag("--flag", "-f", "flag", "desc with `tick` and \"quote\"", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bashOs, zshOs, fishOs, psOs;
    root.printCompletionBash(bashOs);
    root.printCompletionZsh(zshOs);
    root.printCompletionFish(fishOs);
    root.printCompletionPowerShell(psOs);
    const std::string bashScript = bashOs.str();
    const std::string zshScript = zshOs.str();
    const std::string fishScript = fishOs.str();
    const std::string psScript = psOs.str();

    // The generated fish -n condition must contain each subcommand word as a
    // fully single-quoted literal so it survives the second evaluation
    // without executing command substitutions in any fish evaluation context.
    expect(contains(fishScript, R"fish('__fish_seen_subcommand_from \'don\\\'t\' \'$x\' \'`cmd`\' \'$(cmdx)\' \'(paren)\' \'bs\\\\\' \'bs\\\\\\\\\' \'a\\\\\\\\b\'')fish"),
           "e2e fish: condition quotes apostrophe/dollar/substitution/backslash words");
    expect(contains(fishScript, R"(-d "desc with `tick` and \"quote\"")"),
           "e2e fish: backtick in description stays literal");

    // Use a random, mode-0700 temp directory so the test cannot be raced by a
    // pre-created predictable path and its scripts are cleaned up on exit.
    std::error_code ec;
    auto base = std::filesystem::temp_directory_path(ec);
    if (ec) base = std::filesystem::current_path(ec);
    std::string tmpl = (base / "clasp_e2e_XXXXXX").string();
    std::vector<char> dirBuf(tmpl.begin(), tmpl.end());
    dirBuf.push_back('\0');
    char* madeDir = ::mkdtemp(dirBuf.data());
    expect(madeDir != nullptr, "e2e: mkdtemp creates private temp dir");
    if (!madeDir) return;
    const std::string dir = madeDir;
    const std::string bashPath = dir + "/comp.bash";
    const std::string zshPath = dir + "/comp.zsh";
    const std::string fishPath = dir + "/comp.fish";
    const std::string fishTestPath = dir + "/check.fish";

    if (haveBash) {
        std::ofstream bf(bashPath, std::ios::binary);
        bf << bashScript;
        bf.close();
        // Interactive bash: source must not error. The script has no `!`, so
        // history expansion does not actually fire here; the pipeline only
        // detects a syntax error (or a non-interactive failure) in the
        // generated script. The pipeline exit code is the source result, not
        // the trailing echo, so a syntax failure is actually detected.
        const std::string cmd =
            "printf 'source \"%s\" && echo SRC_OK || { echo SRC_FAIL; exit 1; }\\n' \""
            + bashPath + "\" | bash --norc -i >/dev/null 2>&1";
        expect(std::system(cmd.c_str()) == 0, "e2e bash: script sources without syntax error");
    }

    if (haveZsh) {
        std::ofstream zf(zshPath, std::ios::binary);
        zf << zshScript;
        zf.close();
        // Export ZDOTDIR (not a prefix assignment) so compinit actually
        // inherits it and writes .zcompdump into the isolated dir instead of
        // $HOME; HOME is also redirected so no user dir is polluted. The test
        // fails unless .zcompdump lands in the isolated dir and not in HOME.
        const std::string zshHome = dir + "/home";
        const bool zshHomeCreated = std::filesystem::create_directories(zshHome, ec);
        expect(!ec && zshHomeCreated, "e2e zsh: isolated HOME directory created without error");
        const std::string cmd = "zsh -fc 'export HOME=\"" + zshHome + "\"; export ZDOTDIR=\"" + dir
                                + "\"; autoload -Uz compinit && compinit >/dev/null 2>&1 && test -f \"" + dir
                                + "/.zcompdump\" && test ! -f \"" + zshHome
                                + "/.zcompdump\" && source \"" + zshPath + "\" >/dev/null 2>&1'";
        expect(std::system(cmd.c_str()) == 0, "e2e zsh: compinit isolated via exported ZDOTDIR + source succeed");
    }

    if (haveFish) {
        std::ofstream ff(fishPath, std::ios::binary);
        ff << fishScript;
        ff.close();
        std::ofstream tf(fishTestPath, std::ios::binary);
        tf << "source \"" << fishPath << "\"\n";
        // Marker commands: if any command-substitution shape is executed on the
        // second evaluation, its output replaces the literal word in the
        // generated completion list, which the assertions below detect.
        tf << "function cmd; echo CMD_EXECUTED_BACKTICK; end\n";
        tf << "function cmdx; echo CMD_EXECUTED_CMDX; end\n";
        tf << "function paren; echo CMD_EXECUTED_PAREN; end\n";
        tf << "set -l subs (complete -C \"e2eapp \")\n";
        tf << "if not contains -- \"don't\" $subs; exit 1; end\n";
        tf << "if not contains -- '$x' $subs; exit 1; end\n";
        tf << "if not contains -- '`cmd`' $subs; exit 1; end\n";
        tf << "if not contains -- '$(cmdx)' $subs; exit 1; end\n";
        tf << "if not contains -- '(paren)' $subs; exit 1; end\n";
        tf << "if not contains -- 'bs\\\\' $subs; exit 1; end\n";
        // Even backslash counts must round-trip intact through the second
        // evaluation: `bs\\` (two trailing) and `a\\b` (two in the middle)
        // would otherwise be silently collapsed to one backslash each.
        tf << "if not contains -- 'bs\\\\\\\\' $subs; exit 1; end\n";
        tf << "if not contains -- 'a\\\\\\\\b' $subs; exit 1; end\n";
        tf << "if contains -- CMD_EXECUTED_BACKTICK $subs; exit 1; end\n";
        tf << "if contains -- CMD_EXECUTED_CMDX $subs; exit 1; end\n";
        tf << "if contains -- CMD_EXECUTED_PAREN $subs; exit 1; end\n";
        tf << "set -l apos (complete -C \"e2eapp don\\'t --\")\n";
        tf << "if not string match -q -- \"--flag*\" $apos; exit 1; end\n";
        tf << "set -l x OTHERVAL\n";
        tf << "set -l literal (complete -C 'e2eapp \\$x --')\n";
        tf << "if not string match -q -- \"--flag*\" $literal; exit 1; end\n";
        tf << "set -l dollar (complete -C \"e2eapp $x --\")\n";
        tf << "if string match -q -- \"--flag*\" $dollar; exit 1; end\n";
        // Command-substitution-shaped tokens must stay literal in the -n
        // condition too: each enables the local flag without running the
        // substitution (else the condition word no longer matches and the
        // flag is not offered, and the marker output would leak).
        tf << "set -l back (complete -C 'e2eapp `cmd` --')\n";
        tf << "if not string match -q -- \"--flag*\" $back; exit 1; end\n";
        tf << "set -l dpar (complete -C 'e2eapp $(cmdx) --')\n";
        tf << "if not string match -q -- \"--flag*\" $dpar; exit 1; end\n";
        tf << "set -l par (complete -C 'e2eapp (paren) --')\n";
        tf << "if not string match -q -- \"--flag*\" $par; exit 1; end\n";
        // The trailing-backslash subcommand must reach the -n condition as the
        // literal token `bs\` (the commandline spells it `bs\\\\` so fish
        // tokenizes one backslash, not an escaped space) and enable the flag.
        tf << "set -l bsq (complete -C 'e2eapp bs\\\\\\\\ --')\n";
        tf << "if not string match -q -- \"--flag*\" $bsq; exit 1; end\n";
        // The even-backslash aliases must reach the -n condition as their
        // literal two-backslash tokens and enable the flag too. `complete -C`
        // re-tokenizes its argument, so the command line must carry eight
        // backslash bytes (`bs\\\\\\\\` / `a\\\\\\\\b` in the source) to
        // collapse to the two-backslash token fish compares against.
        tf << "set -l bsq2 (complete -C 'e2eapp bs\\\\\\\\\\\\\\\\ --')\n";
        tf << "if not string match -q -- \"--flag*\" $bsq2; exit 1; end\n";
        tf << "set -l amid (complete -C 'e2eapp a\\\\\\\\\\\\\\\\b --')\n";
        tf << "if not string match -q -- \"--flag*\" $amid; exit 1; end\n";
        tf.close();
        // The fish script must parse, and the -n condition must be re-evaluated
        // at runtime: `don't` must enable the local flag, while `$x` must stay
        // literal and therefore not enable it.
        const std::string cmd = "fish -N \"" + fishTestPath + "\" >/dev/null 2>&1";
        expect(std::system(cmd.c_str()) == 0, "e2e fish: -n condition re-evaluates apostrophe and dollar");
    }

    // Cleanup must succeed: a silent /tmp leak is exactly what the earlier
    // mkdtemp/mkdir change was meant to fix, so surface any failure here.
    const std::uintmax_t removed = std::filesystem::remove_all(dir, ec);
    expect(!ec && removed != 0, "e2e: temp dir removed without error");
#endif
}

// ------------------------------------------------------------------
// enableCompletion(): registration variants.
// ------------------------------------------------------------------

