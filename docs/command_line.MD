# Command Line Arguments

The IceBro Lite executable accepts command line arguments for automation and initialization.

* `-load=<file>`: load a program such as a `.prg`, `.d64` or `.crt` once VICE is connected. IceBro Lite will also look for matching debug files with the same base name.
* `-symbols=<file>`: load symbols from a `.dbg`, `.sym` or `.vs` file immediately.
* `-connect`: connect to an already-running VICE instance.
* `-start`: start the previously configured VICE executable on startup.
* `-start=<file>`: start a specific VICE executable.
* `-font=<file>,<size>`: load an alternate font. Example: `-font=C:\Windows\Fonts\cour.ttf,20`.
* `-extradebug=<file>`: load an additional debug XML file in addition to the program debug info.
* `-emu=<type>`: start a VICE machine where `<type>` is `c64`, `vic20` or `plus4`.
* `-help`, `-?` or `-info`: print the supported options.

