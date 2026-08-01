# Start Debugging

The debugger updates its view of the machine state when VICE is paused. You can stop VICE by clicking the Pause/Go button in the toolbar, pressing Shift+F5, hitting Alt+H in VICE, or triggering a breakpoint or watchpoint.

## Starting a session

You can begin debugging in either of these ways:
1. Start VICE first and then connect to it from IceBro Lite.
2. Start the debugger first, then load a program from within IceBro Lite and let it launch or attach to VICE.

## Loading a program

Use the Load button in the toolbar to open a `.prg`, `.d64` or `.crt` file. IceBro Lite will try to locate matching debug data in the same folder and load symbols and source information automatically.

The Reload button repeats the most recent load. Reload Debug Info reloads the matching debug information without reloading the program itself.

## Useful workflow tips

- Drag symbols from the Symbol View into the Code, Memory or Breakpoint views to inspect or break on them.
- Use the watch view to monitor expressions live while the emulator is stopped.
- Right-click in the code view to add a watch entry or jump to another address.

## Keyboard shortcuts

Global shortcuts:
* **F5**: go/run
* **Shift+F5**: pause/break
* **F11**: step into
* **F10**: step over
* **Shift+F11**: step out
* **Esc**: exit edit mode

Code view shortcuts:
* **Tab**: jump to the current PC
* **Shift+Tab**: set the PC to the currently selected address
* **F9**: toggle a breakpoint at the selected line
* **Enter**: edit the selected address

Watch shortcuts:
* **Enter**: edit the active watch expression
* **Delete**: remove the active watch entry
* **Insert**: insert a new watch entry
* **Up/Down**: move the active selection

