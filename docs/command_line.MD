# Command Line Arguments

The IceBroLite.exe executable accepts command line arguments to speed up getting started and for additional initialization.

* -load=\<executable>: load a .prg, .d64, .crt etc. immediately after VICE is connected. This will also load any .dbg / .sym / .vs file with the same filename as the executable.
* -symbols=\<.dbg/.sym/.vs>: load this symbol file
* -connect: automatically connect with a running VICE executable
* -start: start the previously loaded VICE executable on startup
* -start=<file>: start a VICE executable specified as the file
* -font=\<.ttf/.otf file\>,\<size\>: load an alternative font, for example -font=c:\windows\fonts\cour.ttf,20
* -extradebug=<file>: attempt to load an additional debug XML file (kickasm) in addition to the program debug
* -emu=<type>: start a VICE emulator where type is one of: c64, vic20 or plus4
