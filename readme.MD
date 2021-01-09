# IceBro Lite

This is a debugger in development for the new external binary monitor protocol in Vice 3.5. It is not ready for use quite yet.

### Differences from IceBro

IceBro Lite doesn't contain its own 6502 emulator for debugging "post mortem" which makes it a lot simpler to understand. When it is ready it should function similar to the command line monitor of vice only with a graphical user interface and more visualizations of the current state.

### Status

Currently stuck trying to communicate at a reasonable rate with Vice. This is being looked at.

## Quick Usage Info

* This tool is built on dear ImGui docking branch and uses the docking feature. Drag windows onto guides in the main window to set them up as panes.
* F10 = Step Over, F11 = Step Into, Shift+F11 = Step Out (after next rts), F5 = Go, F6 = Run until cursor (from Code View)
* Disassembler seems a bit broken right now, mostly works.
* Any addrews field can be filled out with expressions, hex numbers require $ prefix
* {addr} evaluates to a 16 bit value from memory adress
* \[addr\] evaluates to an 8 bit value from memory address
* = prefix in address fields locks the expression, for instance ={$fffe} in a code view will always disassemble the current interrupt code
* Expressions can be complex, contain parenthesis, etc. et.
* To enable the VICE Console make sure the text monitor interface is enabled as well (port 6510)
* Load Kick Assembler debug data from the menu bar
* Click the _Load_ icon in the ToolBar to specify a .prg or .d64 or .crt file to start in VICE (right now it exits back into the debugger immediately AND when then program has loaded, just hit F5 to continue)
 * Click the _Reload_ icon to reload the program
* Enter math expressions in Watch View, prefix with \* to view bytes at address, for example *$101+s to see what has been pushed on the stack
  
### Building IceBroLite on Linux

(I'm using this on a Raspberry PI 400, still need to verify features with RPi400 Vice 3.5)

* Install glfw (sudo apt-get install -y libglfw3-dev)
* Go to the src folder and run make
* You should be able to run IceBroLite.exe from the root folder.

### Planned features

(these are pretty much just going to be copied over from IceBro, with fixes as needed)

* Load symbols or Vice listings for symbols & breakpoints
* Listing files for source level debugging
* off-screen graphics
* Reset & Load (Reload) VICE d64 or prg
* Breakpoint view

