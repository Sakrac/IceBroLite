# Command Line Arguments

The IceBroLite.exe executable accepts command line arguments to speed up getting started and for additional initialization.

* -load=\<executable>: load a .prg, .d64, .crt etc. immediately after VICE is connected. This will also load any .dbg / .sym / .vs file with the same filename as the executable.
* -symbols=\<.dbg/.sym/.vs>: load this symbol file
* -connect: automatically connect with a running VICE executable
* -start: (optionally with a path to a VICE executable, for example -start=x64sc.exe) automatically start VICE
* -font=\<.ttf/.otf file\>,\<size\>: load an alternative font, for example -font=c:\windows\fonts\cour.ttf,20
