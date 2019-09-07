# Robust Services Core: Diplomacy Bot

Much of the software in the [*dip*](/dip) directory was evolved from the base software
for developing Diplomacy bots found on the [DAIDE](http://www.daide.org.uk) (Diplomacy
AI Development Environment) website.  That software was refactored, decoupled from
Windows (to run as a console application), and evolved to introduce C++11 capabilities.

The [*extras*](/dip/extras) directory should give you an idea of the current state of
this software.  To build a bot that plays the game, derive from [`BaseBot`](/dip/BaseBot.h)
and follow the instructions in [*BotType.h*](/dip/BotType.h).  If you simply build the
software as is, it produces an observer bot that can join a game and report on its progress.
Several files in the *extras* directory illustrate the output from this observer.  All the
games in that directory were played by instances of the impressive Albert bot:

* A win by Turkey in a [standard](/dip/extras/Standard-TUR-win.txt) game.
* A draw between England, France, and Austria in a
[fleet_rome](/dip/extras/Fleet-Rome-AUS-ENG-FRA-draw.txt) game.
* The first season of a [classical](/dip/extras/classical.console.txt) game.

The last game is included because it is accompanied by a [trace](/dip/extras/classical.trace.txt)
of the game's startup and initial season.  This should give you an idea of the debugging
capabilities available in RSC.  The last thing in the *extras* directory is a Win32 debug
executable of the observer bot (*obsbot.exe*).  If you launch it with the `-L3` option, it will
capture the level of information shown in that trace.  The trace is saved to a file using
the `>save` command.  CLI commands are documented [here](/output/help.cli.txt), but only
those available in the `NodeBase` and `NetworkBase` namespaces are available to a bot.  If
you start it with `-L2`, the function trace is omitted.  Launching with `-L1` also omits
socket events, leaving only the messages.  Finally, launching with `-L0` will capture no
debug information.

## Building the executable

To build the observer bot, or even subclass it to create a bot that plays the game, start
with the instructions on the [main page](/README.md).  However, the observer bot is not
enabled in a default RSC build.  But it is easy to target the build for a bot by modifying
[`main`](/rsc/main.cpp) as follows:

* In the list of `#include` directives, comment out all `...Module.h` entries and uncomment
the one for `DipModule.h`.
* In the list of `using` directives, comment out all of them and uncomment the one for
`namespace Diplomacy`.
* In the list of `Singleton` invocations, comment out all of them and uncomment the one
for `DipModule`.

If you've already developed a bot, rebasing it onto RSC will require a bit of work because
some of the interfaces have changed.  However, it shouldn't prove too difficult.  Note that
only the software exercised by the observer bot has been tested; `Adjudicator.cpp` probably
contains a few surprises at this stage.
