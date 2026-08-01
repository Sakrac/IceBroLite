# Updated features

This page summarizes the main improvements that are reflected in the current manual.

## Command line options

IceBro Lite accepts command line arguments for startup automation and initialization:

* `-load=<file>`: load a program and matching debug files automatically
* `-symbols=<file>`: load symbols from a debug file
* `-connect`: connect to an already-running VICE instance
* `-start` / `-start=<file>`: launch VICE at startup
* `-font=<file>,<size>`: override the UI font
* `-extradebug=<file>`: load additional debug data
* `-emu=<type>`: select a VICE machine type (`c64`, `vic20`, `plus4`)

## UI and view updates

* Right-click in the watch value column to switch the display between hexadecimal, decimal and binary.
* Right-click in the code view to add a watch entry or jump to a referenced address.
* Hold down the middle mouse button over code or memory views to scroll the contents.
* The code view has a Track PC checkbox to keep the current instruction roughly centered as execution advances.

