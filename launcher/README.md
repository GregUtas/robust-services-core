# Launcher

This directory contains a simple application (_RscLauncher_) that
forks _rsc.exe_ as a child process to support automated reboots
(`RestartReboot`). If RSC requires a reboot, it exits with a
non-zero exit code, and _RscLauncher_ immediately reboots it.

You only need to launch RSC this way to test _RscLauncher_ or if a
deployment requires automated reboots.

When you build RSC, you will discover a _launcher.exe_ as well as an
_rsc.exe_.

To run _RscLauncher_, right-click on the _launcher_ directory and select
_Debug > Start New Instance_. _RscLauncher_ prompts for the directory
where your _rsc.exe_ is located and its command line parameters
(`Prot_kBs=8192` at the time of writing). Note that the executable's
name is currently hard-coded and _must_ be _rsc.exe_.

When RSC starts up, it runs in the same console window as _RscLauncher_.

To debug RSC when it is running, use the VS command _Debug > Attach
to Process..._ and find _rsc.exe_ in the window that pops up.

To shut down RSC, use RSC's CLI command `>restart exit`, which returns
control to _RscLauncher_ without rebooting RSC. If you simply close the
console window or exit _RscLauncher_ using the VS command _Debug > Stop
Debugging_, _rsc.exe_ continues to run, and you will need to use the
Task Manager to kill it. If you start another instance of RSC before
doing this, the new instance generates a series of logs as it fails to
bind to the IP ports that the previous instance of RSC still owns.