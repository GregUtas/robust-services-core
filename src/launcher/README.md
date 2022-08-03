# Launcher

This directory contains a simple application (`Launcher`) that forks
an RSC application as a child process to support automated reboots
(`RestartReboot`). If RSC decides to reboot, it exits with a non-zero
exit code and `Launcher` immediately reboots it.

You only need to launch an RSC application this way to test `Launcher`
or if a deployment requires automated reboots.

When you build RSC, you will find a _launcher.exe_ as well as an
_rsc.exe_ and _rscapp.exe_ (or simply _launcher_, _rsc_, and _rscapp_
if you built for Linux).

To run `Launcher`, right-click on the _launcher_ directory and select
_Debug > Start New Instance_. `Launcher` prompts for the path to your
executable, followed by its command line parameters. Currently, RSC
itself does not use any additional command line parameters.

When the executable starts, it runs in the same console window as
`Launcher`.

To debug the executable once it is running, use the VS command
_Debug > Attach to Process..._ and find the executable in the window
that pops up.

To shut down the executable, use RSC's CLI command `>restart exit`,
which returns control to `Launcher` without rebooting. If you simply
close the console window or exit `Launcher` using the VS command
_Debug > Stop Debugging_, the executable continues to run, and you will
have to use the Task Manager to kill it. If you start another instance
of the executable before doing this, the new instance will generate a
series of logs as it fails to bind to the IP ports that the previous
instance still owns.
