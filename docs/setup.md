# Setup

IceBro Lite connects to VICE over the remote monitor protocols. The setup is straightforward once VICE is configured to expose those interfaces.

## Quick start

1. Start IceBro Lite and click the toolbar button to launch VICE, or connect to an already-running VICE instance.
2. On first launch, point the debugger at your VICE executable if it is not already known.
3. Load a program with the Load button. IceBro Lite will look for matching `.dbg`, `.sym` or `.vs` files automatically.

## VICE configuration

In VICE, enable:
- Remote Monitor
- Binary Remote Monitor

These settings are typically enabled once and then persisted. If you want to re-enable the built-in VICE monitor later, turn the remote monitor options off again.

## Layout files

The debugger stores its window layout and recent state in `icebrolt.ini`. Launching IceBro Lite from a project folder makes it easier to keep a project-specific layout and avoid mixing settings between projects.

## Troubleshooting

- If the debugger cannot connect, make sure VICE is still running and the remote monitor options are on.
- If you start VICE from within IceBro Lite, it will use the configured executable path from the previous run or the one supplied on the command line.

