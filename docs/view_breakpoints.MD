# Breakpoint View

The Breakpoint View is a list of the current checkpoints which includes breakpoints (stop on execution of address), watchpoints (stop when reading or writing to an address or range), tracepoints (tracks loading/storing/execution but does not stop VICE).

![Breakpoint View](img/BreakpointView.png)

* [back to manual](index.md)
* [back to views](views.md)

While debugging code it is usually easiest to toggle breakpoints from the code view, at the top of the Breakpoint View there is a drop down to create other types of checkpoints.

Breakpoint addresses can be dragged from the breakpoint table into the Code tab to see the code at the breakpoint, or to the memory view along with anything you can drag a symbol to.

In the table a checkpoint can be selected, conditions can be added, clicking on a checkpoint icon disables/enables the checkpoint.

Keyboard shortcuts when a checkpoint is selected:
* cursor up/down: move to previous/next checkpoint in table
* Delete: delete the selected checkpoint

