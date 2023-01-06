### History
A tool doesn't exist without an initial use case, and this tool exists because I created a 6502 assembler and I needed a way to step through the generated code. The assembler was created to test a text parsing library I was creating to build game tools at Big Red Button.

The Initial debugger iteration was Step6502 and it had an internal 6502 simulator. Step6502 was created using Microsoft MFC which is highly unstable and useless for something as extensive as a debugger so when MonstersGoBoom created a theme for dear ImGui I immediately started over with that and that became IceBro.

IceBro added support for connecting to the Remote Text Monitor in VICE while retaining the internal 6502 simulator. This became overly complex and hard to work with.

Eventually the Binary Monitor was introduced in VICE 3.5 which was tempting but I didn't switch over until Bacchus/FLT reached out and suggested I do that. In order to clean up the codebase the internal 6502 simulator was dropped and major parts of the code was rewritten.
