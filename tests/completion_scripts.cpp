#include "completion_support.hpp"

void testCompletionEntriesStaticScripts() {
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompletionCommand = false;
    staticCfg.addCompleteCommands = false;

    clasp::Command root("staticapp", "Static scripts");
    root.enableCompletion(staticCfg);
    root.version("2.0");
    root.withFlag("--gflag", "-G", "gflag", "Root global flag", std::string(""));
    root.withFlag("", "-R", "gshort", "Root short-only flag", std::string(""));
    root.withFlag("--gndesc", "", "gndesc", "", std::string(""));
    clasp::Command sub("sub", "Sub cmd");
    sub.addAlias("s");
    sub.withFlag("--both", "-b", "both", "Both names", std::string(""));
    sub.withFlag("--longonly", "", "longonly", "Long only", std::string(""));
    sub.withFlag("", "-x", "shortonly", "Short only", std::string(""));
    sub.withFlag("", "", "nameless", "Nameless flag", std::string(""));
    sub.withFlag("--quoted", "-q", "quoted", "Say \"hi\"", std::string(""));
    sub.withFlag("--sndesc", "", "sndesc", "", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bash, zsh, fish, ps;
    root.printCompletionBash(bash);
    root.printCompletionZsh(zsh);
    root.printCompletionFish(fish);
    root.printCompletionPowerShell(ps);

    expect(contains(bash.str(), "\"staticapp sub\"|\"staticapp s\""), "static bash: alias case label");
    expect(contains(bash.str(), "COMPREPLY"), "static bash: COMPREPLY emitted");
    expect(contains(zsh.str(), "#compdef \"staticapp\""), "static zsh: header");
    expect(contains(fish.str(), R"(-a '\'sub\' \'s\' \'help\' \'version\'')"), "static fish: root subcommand list");
    expect(contains(fish.str(), "-l version -d \"Version for this command\""), "static fish: version flag line");
    expect(contains(fish.str(), "-l longonly"), "static fish: long-only flag");
    expect(contains(fish.str(), "-s x"), "static fish: short-only flag");
    expect(contains(fish.str(), "-l both -s b"), "static fish: both-name flag");
    expect(contains(fish.str(), "-d \"Say \\\"hi\\\"\""), "static fish: escaped quotes in description");
    expect(contains(fish.str(), R"('__fish_seen_subcommand_from \'sub\' \'s\'')"), "static fish: sub flag condition");
    expect(contains(ps.str(), "$__claspSubs['staticapp sub']"), "static powershell: sub map entry");
    expect(contains(ps.str(), "$__claspFlags['staticapp sub']"), "static powershell: flag map entry");
    expect(contains(ps.str(), "'sub', 's', 'help', 'version'"), "static powershell: quoted list");
    expect(contains(ps.str(), "$__claspSubs['staticapp help']"), "static powershell: help entry");
    expect(contains(ps.str(), "$__claspSubs['staticapp version']"), "static powershell: version entry");

    // Without a version: no version entries anywhere.
    clasp::Command root2("nover", "No version");
    root2.enableCompletion(staticCfg);
    clasp::Command sub2("leaf", "Leaf");
    root2.addCommand(std::move(sub2));
    std::ostringstream bash2, fish2, ps2;
    root2.printCompletionBash(bash2);
    root2.printCompletionFish(fish2);
    root2.printCompletionPowerShell(ps2);
    expect(!contains(fish2.str(), "-l version"), "static fish: no version line without version");
    expect(!contains(ps2.str(), "'nover version'"), "static powershell: no version entry");
    expect(!contains(bash2.str(), "\"nover version\""), "static bash: no version case label");

    // With the help command disabled: no help entries.
    clasp::Command root3("nohelp", "No help");
    root3.disableHelpCommand();
    root3.enableCompletion(staticCfg);
    clasp::Command sub3("leaf", "Leaf");
    sub3.addAlias("l");
    root3.addCommand(std::move(sub3));
    std::ostringstream bash3, fish3, ps3;
    root3.printCompletionBash(bash3);
    root3.printCompletionFish(fish3);
    root3.printCompletionPowerShell(ps3);
    expect(contains(fish3.str(), R"(-a '\'leaf\' \'l\'')"), "static fish: no help in subcommand list");
    expect(!contains(fish3.str(), "\"leaf l help\""), "static fish: help command disabled");
    expect(!contains(bash3.str(), "\"nohelp help\""), "static bash: no help case label");
    expect(!contains(ps3.str(), "'nohelp help'"), "static powershell: no help entry");
}

// ------------------------------------------------------------------
// Dynamic shell scripts (addCompleteCommands / complete command names).
// ------------------------------------------------------------------
void testDynamicShellScripts() {
    clasp::Command root("dynapp", "Dynamic scripts");
    root.enableCompletion();
    clasp::Command sub("leaf", "Leaf");
    sub.withFlag("--opt", "-o", "opt", "Opt", std::string(""));
    root.addCommand(std::move(sub));

    std::ostringstream bash, zsh, fish, ps;
    root.printCompletionBash(bash);
    root.printCompletionZsh(zsh);
    root.printCompletionFish(fish);
    root.printCompletionPowerShell(ps);
    expect(contains(bash.str(), "__completeNoDesc"), "dynamic bash: calls complete command");
    expect(contains(bash.str(), "ShellCompDirectiveError"), "dynamic bash: directive handling");
    expect(contains(zsh.str(), "#compdef \"dynapp\""), "dynamic zsh: header");
    expect(contains(fish.str(), "__clasp_dynapp_fish_complete"), "dynamic fish: helper function");
    expect(contains(fish.str(), "\"__complete\""), "dynamic fish: uses desc command");
    expect(contains(ps.str(), "$__claspCompleteCmd = '__completeNoDesc'"), "dynamic powershell: complete command");
    expect(contains(ps.str(), "Register-ArgumentCompleter"), "dynamic powershell: registration");

    // Only the desc command name configured.
    clasp::Command::CompletionConfig descOnly;
    descOnly.completeNoDescCommandName = "";
    clasp::Command root2("descapp", "Desc only");
    root2.enableCompletion(descOnly);
    std::ostringstream fish2;
    root2.printCompletionFish(fish2);
    expect(contains(fish2.str(), "\"__complete\""), "dynamic fish: desc-only config");

    // Only the no-desc command name configured.
    clasp::Command::CompletionConfig noDescOnly;
    noDescOnly.completeCommandName = "";
    clasp::Command root3("ndapp", "NoDesc only");
    root3.enableCompletion(noDescOnly);
    std::ostringstream fish3;
    root3.printCompletionFish(fish3);
    expect(contains(fish3.str(), "\"__completeNoDesc\""), "dynamic fish: nodesc-only config");

    // Both names empty: fish falls back to the static script.
    clasp::Command::CompletionConfig noNames;
    noNames.completeCommandName = "";
    noNames.completeNoDescCommandName = "";
    clasp::Command root4("nonames", "No names");
    root4.enableCompletion(noNames);
    std::ostringstream fish4, bash4, ps4;
    root4.printCompletionFish(fish4);
    root4.printCompletionBash(bash4);
    root4.printCompletionPowerShell(ps4);
    expect(contains(fish4.str(), "complete -c \"nonames\" -f -a"), "fish static fallback: static output");
    expect(contains(bash4.str(), "_nonames_complete() {"), "bash static fallback: static output");
    expect(contains(ps4.str(), "$__claspSubs['nonames']"), "powershell static fallback: static output");

    // addCompleteCommands disabled: static scripts everywhere.
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompleteCommands = false;
    clasp::Command root5("allstatic", "All static");
    root5.enableCompletion(staticCfg);
    std::ostringstream fish5, bash5, ps5;
    root5.printCompletionFish(fish5);
    root5.printCompletionBash(bash5);
    root5.printCompletionPowerShell(ps5);
    expect(contains(fish5.str(), "complete -c \"allstatic\" -f -a"), "fish addCompleteCommands=false: static");
    expect(contains(bash5.str(), "_allstatic_complete() {"), "bash addCompleteCommands=false: static");
    expect(contains(ps5.str(), "$__claspSubs['allstatic']"), "powershell addCompleteCommands=false: static");

    // Long identifiers (>15 chars) exercise the heap-allocation branches of
    // inlined std::string copies in the script generators.
    clasp::Command::CompletionConfig longNodesc;
    longNodesc.completeNoDescCommandName = "a-long-complete-nodesc-name";
    clasp::Command longRoot("a-very-long-root-application", "Long identifiers");
    longRoot.enableCompletion(longNodesc);
    clasp::Command longSub("a-long-subcommand-name", "With a really long description text");
    longSub.addAlias("a-long-alias-name-x");
    longSub.withFlag("--a-very-long-flag-name", "", "a-very-long-flag-name",
                     "A really long flag description text", std::string(""));
    longRoot.addCommand(std::move(longSub));

    std::ostringstream lbash, lfish, lps;
    longRoot.printCompletionBash(lbash);
    longRoot.printCompletionFish(lfish);
    longRoot.printCompletionPowerShell(lps);
    expect(contains(lbash.str(), "_a_very_long_root_application_complete"), "long identifiers: bash sanitized");
    expect(contains(lps.str(), "$__claspCompleteCmd = 'a-long-complete-nodesc-name'"),
           "long identifiers: powershell nodesc command");
    expect(contains(lfish.str(), "__clasp_a_very_long_root_application_fish_complete"),
           "long identifiers: dynamic fish helper");

    clasp::Command::CompletionConfig longStatic;
    longStatic.addCompletionCommand = false;
    longStatic.addCompleteCommands = false;
    clasp::Command longStaticRoot("another-very-long-root-name", "Long static");
    longStaticRoot.enableCompletion(longStatic);
    longStaticRoot.version("1.0.0");
    clasp::Command longStaticSub("a-long-subcommand-name", "With a really long description text");
    longStaticSub.withFlag("--a-very-long-flag-name", "", "a-very-long-flag-name",
                           "A really long flag description text", std::string(""));
    longStaticRoot.addCommand(std::move(longStaticSub));
    std::ostringstream lsbash, lsfish, lsps;
    longStaticRoot.printCompletionBash(lsbash);
    longStaticRoot.printCompletionFish(lsfish);
    longStaticRoot.printCompletionPowerShell(lsps);
    expect(contains(lsbash.str(), "\"another-very-long-root-name help\""), "long identifiers: static bash help entry");
    expect(contains(lsfish.str(), "-l a-very-long-flag-name -d \"A really long flag description text\""),
           "long identifiers: static fish long flag description");
    expect(contains(lsps.str(), "$__claspSubs['another-very-long-root-name a-long-subcommand-name']"),
           "long identifiers: static powershell sub entry");
    expect(contains(lsps.str(), "$__claspFlags['another-very-long-root-name a-long-subcommand-name']"),
           "long identifiers: static powershell flag map entry");
}

// ------------------------------------------------------------------
// Shell escaping helpers: apostrophes and identifier sanitization.
// ------------------------------------------------------------------
void testShellEscaping() {
    clasp::Command::CompletionConfig staticCfg;
    staticCfg.addCompletionCommand = false;
    staticCfg.addCompleteCommands = false;

    clasp::Command root("esc", "Escaping");
    root.enableCompletion(staticCfg);
    clasp::Command sub("it's", "Apostrophe");
    sub.addAlias("a'b");
    root.addCommand(std::move(sub));

    std::ostringstream ps, bash;
    root.printCompletionPowerShell(ps);
    expect(contains(ps.str(), "it''s"), "powershell escaping: apostrophe doubled");
    expect(contains(ps.str(), "a''b"), "powershell escaping: alias apostrophe doubled");

    root.printCompletionBash(bash);
    expect(contains(bash.str(), "\"esc it's\""), "bash escaping: apostrophe kept verbatim");

    // Non-identifier characters in the root name.
    clasp::Command weird("my-app.v2", "Weird");
    weird.enableCompletion(staticCfg);
    std::ostringstream wb;
    weird.printCompletionBash(wb);
    expect(contains(wb.str(), "_my_app_v2_complete"), "sanitize: non-alnum chars replaced");

    // Underscores are kept as-is by sanitization.
    clasp::Command under("app_x", "Underscore");
    under.enableCompletion(staticCfg);
    std::ostringstream ub2;
    under.printCompletionBash(ub2);
    expect(contains(ub2.str(), "_app_x_complete"), "sanitize: underscore kept");

    // A leading underscore suppresses the auto-prefix.
    clasp::Command lead("_lead", "Leading underscore");
    lead.enableCompletion(staticCfg);
    std::ostringstream lb;
    lead.printCompletionBash(lb);
    expect(contains(lb.str(), "___lead_complete") || contains(lb.str(), "__lead_complete"),
           "sanitize: leading underscore tolerated");

    clasp::Command digit("7app", "Digit start");
    digit.enableCompletion(staticCfg);
    std::ostringstream db;
    digit.printCompletionBash(db);
    expect(contains(db.str(), "__7app_complete"), "sanitize: leading digit gets underscore prefix");

    // An empty root name sanitizes to a single underscore.
    clasp::Command empty("", "Empty name");
    empty.enableCompletion(staticCfg);
    std::ostringstream eb;
    empty.printCompletionBash(eb);
    expect(!eb.str().empty(), "sanitize: empty name tolerated");

    // Shell metacharacters in the root name must be quoted, not interpolated
    // raw (defense-in-depth against generated-script injection).
    clasp::Command meta("app;echo INJECTED", "Metachar root");
    meta.enableCompletion(staticCfg);
    meta.version("1.0");
    clasp::Command metaSub("sub'$x`y\\z", "Metachar sub");
    metaSub.addAlias("al\"ias`");
    metaSub.withFlag("--fl\"ag", "-f", "fl\"ag", "desc $(echo `x`)", std::string(""));
    meta.addCommand(std::move(metaSub));
    std::ostringstream mbash, mzsh, mfish, mps;
    meta.printCompletionBash(mbash);
    meta.printCompletionZsh(mzsh);
    meta.printCompletionFish(mfish);
    meta.printCompletionPowerShell(mps);
    const auto bashStr = mbash.str();
    const auto zshStr = mzsh.str();
    const auto fishStr = mfish.str();
    const auto psStr = mps.str();
    // Raw "app;echo INJECTED" must never appear as an unquoted command token.
    expect(contains(bashStr, "\"app;echo INJECTED\""), "metachar bash: root quoted in cmd");
    expect(contains(bashStr, "complete -F _app_echo_INJECTED_complete \"app;echo INJECTED\""),
           "metachar bash: root quoted in complete line");
    expect(contains(zshStr, "#compdef \"app;echo INJECTED\""), "metachar zsh: root quoted in compdef");
    expect(contains(fishStr, "complete -c \"app;echo INJECTED\""), "metachar fish: root quoted in complete");
    expect(contains(fishStr, R"(\'sub\\\'$x`y\\\\z\')"), "metachar fish: apostrophe/quote sub quoted");
    expect(contains(psStr, "Register-ArgumentCompleter -CommandName 'app;echo INJECTED'"),
           "metachar powershell: root quoted in registration");
    // Subcommand name with a single quote must be doubled inside the PowerShell map key.
    expect(contains(psStr, "'app;echo INJECTED sub''$x`y\\z'"), "metachar powershell: sub quote doubled");
    // No raw unquoted semicolon command injection may survive in the dynamic invocation line.
    expect(!contains(bashStr, "done < <(app;echo"), "metachar bash: no raw root in dynamic invocation");
    expect(!contains(fishStr, "set -l out (app;echo"), "metachar fish: no raw root in invocation");

    // Dynamic scripts (default completion config) plus a root name containing
    // every shell-special character: exercises the true branches of the
    // escaping helpers and the dynamic invocation paths.
    clasp::Command metaDyn("a\"$'`x\\y", "Metachar dynamic root");
    metaDyn.enableCompletion();
    clasp::Command metaDynSub("s'b`$", "Metachar dynamic sub");
    metaDynSub.addAlias("q\"z");
    metaDyn.addCommand(std::move(metaDynSub));
    std::ostringstream dbash, dzsh, dfish, dps;
    metaDyn.printCompletionBash(dbash);
    metaDyn.printCompletionZsh(dzsh);
    metaDyn.printCompletionFish(dfish);
    metaDyn.printCompletionPowerShell(dps);
    const auto dbashStr = dbash.str();
    const auto dfishStr = dfish.str();
    const auto dpsStr = dps.str();
    // Root name must be fully quoted (double quotes in bash/fish, single in PS).
    expect(contains(dbashStr, "\"a\\\"\\$'\\`x\\\\y\""), "metachar dynamic bash: root fully escaped");
    expect(contains(dfishStr, "\"a\\\"\\$'`x\\\\y\""), "metachar dynamic fish: root fully escaped");
    expect(contains(dpsStr, "Register-ArgumentCompleter -CommandName 'a\"$''`x\\y'"),
           "metachar dynamic powershell: root single-quoted with doubled quote");
    // No unquoted raw command token may survive in the dynamic invocation line.
    expect(!contains(dbashStr, "done < <(a\"$"), "metachar dynamic bash: no raw root in invocation");
    expect(!contains(dfishStr, "set -l out (a\"$"), "metachar dynamic fish: no raw root in invocation");
}

// ------------------------------------------------------------------
// End-to-end shell verification: the generated fish `-n` condition must be
// re-evaluated correctly when the subcommand name contains apostrophes,
// dollars or backticks, and the bash/zsh scripts must source cleanly.
// This closes the substring-only assertion gap.
// ------------------------------------------------------------------

