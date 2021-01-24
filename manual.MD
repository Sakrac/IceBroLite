# IceBro Lite Manual

	Note: This document is still in development. It is a bit longer than I anticipated and there is still more things to add and it will be updated soon. If anything needs more clarification please let me know

## Overview

IceBro Lite is a GUI debugger built for the new binary monitor protocal introduced in VICE C64 3.5. The debugger must first be connected to a running instance of VICE that has enabled the binary monitor and ideally the text mode monitor protocols. More details about the binary protocol can be found here:

https://vice-emu.sourceforge.io/vice_13.html

The debugger will only update the machine state after connecting to VICE and then stopping VICE, either by pressing the pause button in the debugger, hitting Alt+H in VICE or hitting a breaking checkpoint. The primary purpose of this debugger is to validate and inspect assembler code.

The process for debugging and iterating on C64 software is up to the user. The program can be started in VICE first and then attaching the debugger or VICE can be connected to the debugger and then the program can be loaded and reloaded within the debugger.

Earlier versions of VICE are not compatible with this debugger, but if you have a reason to primarily use an earlier version you can safely install VICE 3.5 to a different folder.

There are various other options for debuggers to explore, aside from this software and the built-in VICE monitor. If this software does not handle specific use cases well then other options may suit the problem better. If any improvements could help the processes then there are ways to implement such improvements.

# Setting up VICE for debugging with IceBro Lite

### VICE Settings

To enable external debugging VICE must first be configured to do so. It may be required to revert these settings to enable the built-in monitor again.

First find the settings menu 

![VICESettingsMenu](img/VICESettingsMenu.png)

Check Enable Remote Monitor and Enable Binary Remote Monitor in the Host sub-menu.
Consider saving the settings in the Settings Menu at this point to avoid repeating the procedure at a future point.

![VICESettingsMonitor](img/VICESettings_Host_Monitor.png)

VICE is now ready to accept a binary debugger connection!

# Starting IceBro Lite

Start IceBroLite.exe. I recommend doing so from a command line or batch script in your project folder as the debugger will save its layout in a text file from where it was started. If using source control consider ignoring "icebrolt.ini" if using in different environments as it stores full path names to previously loaded files.

![IceBroLite Default Startup](img/IceBroLite_initial.png)

If VICE 3.5 is configured for a binary remote monitor and running simply click the VICE icon in IceBro Lite to connect.

![Vice Connect Icon](img/VICE_Connect_Icon.png)

If the Icon turns cyan VICE is connected and you can press the pause Icon to look around in the current memory of VICE's C64 instance.


# Dear ImGui Docking concepts

IceBro Lite is built on Dear ImGui Docking Branch which is similar to how for instance Visual Studio organizes windows. You can reorganize the panes any way you like, close ones you don't need and open other ones from the Windows drop down menu.

![Dear ImGui Drag Pane](img/IceBroLite_Drag_Pane.png)

Grab any pane by its tab (the bar with the name of each pane, such as Code, Graphics, Watch, etc.) then drag it around with your mouse. If you drop it on an anchor in another pane it will preview where the pane will be placed if you drop it.

Panes can also be dropped outside of the main window and become detached windows where other panes can be attached if you prefer to work with multiple groupings of windows.

![Reset Layout menu](img/IceBroLite_ResetLayout.png)

Any pane that has been closed can be re-opened from the Windows menu where it was last opened. If any problem with the tool layout occurs it can be reset in the drop down menu.


# Getting started with IceBro Lite

With the connection between VICE and the debugger there are two options for starting code in VICE. Either the traditional method of starting from VICE and then stopping execution into the debugger or by clicking the LOAD icon in IceBro Lite and selecting a .prg/.d64 or .crt file from the file browser.

![IceBro Lite Load Icon](img/IceBroLite_Load_Icon.png)

If you start code from the debugger it will try to find a .dbg file (Kick Assembler debugger data), or .sym file (general assembler symbol file) or .vs file (Vice monitor commands) and load symbols / source debug at the same time.

If you start code in VICE you can browse for the appropriate symbol file from the debugger "File" menu.

![Starting to debug Witch Rescue Service](img/IceBroLite_WitchRescueService.png)

When starting code from the debugger it will stop a couple of times before running the code, first before it starts loading then again when it is about to run the loaded code.

To return to VICE just press F5 or press the play icon in the toolbar.

If you started code from the LOAD icon you can rebuild the code in your external editor/assembler and restart with the RELOAD icon without shutting down VICE. This has been a significant iteration time reduction for me so far.


# Getting to know the GUI

A lot of debugging operations involve taking a symbol or pointer from one pane and seeing it in another. For this purpose you can drag symbols and pointers from for instance the Symbol browser or Breapoints to the tabs of other windows, for instance the Code view, the Breakpoint view or the Memory view. In the image above a symbol form the Symbol pane is dragged to the Breakpoints view, this will add that symbol as a breakpoint in VICE.

### Keyboard controls

Global keys
* **F5**: Return to VICE / Run
* **F11**: Step Into as in step one instruction and follow into jsr instructions
* **F10**: Step Over, same as F11 but don't follow into jsr instructions
* **Esc**: Exit the current edit

Code pane Keys
* **F6**: Run until line with purple cursor is reached or any prior breakpoint
* **F9**: Toggle breakpoint at line with purple cursor
* **Enter**: Edit code at line with purple cursor, Esc or Enter on empty line to stop.

Memory pane Keys (after clicking on a byte)
* **Cursor keys**: Move around the cursor in memory
* **Hex numbers**: Modify value in memory

Breakpoint pane Keys (after clicking on a breakpoint)
* **Up/Down**: Move through breakpoints
* **Delete**: Delete breakpoint


## Code View

...

## Watch View

..

## Console View

..

## Breakpoint View

..

## Memory View

..

## Register View

...

## Screen View

...

## Symbol View

...

## Section View

...



# Acknowledgements

### Dear ImGui
Copyright (c) 2014-2021 Omar Cornut, licensed under the MIT Licence, for more information see src/imgui/LICENSE.txt
Dead ImGui can be downloaded from https://github.com/ocornut/imgui

### C64 Font TTF format

Fonts in this package are(c) 2010 - 2014 Style. Used with permission from Style as an embedded binary font. For more information see the top of src/data/C64_Pro_Mono-STYLE.ttf.cpp

### Default Style by MonstersGoBoom

Palette and color assigments by MonstersGoBoom, used with permission

### VICE C64

This tool does not include any code from VICE C64 but interfaces with it using a network connection. VICE C64 download and related licenses can be found at https://vice-emu.sourceforge.io/