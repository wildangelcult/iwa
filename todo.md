Abych vyresil tento exploit https://www.unknowncheats.me/forum/c-and-c-/390593-vm-escape-via-nmi.html#post2772568 zkopiruju si pro sebe IDT 1:1 akorat zmenim interrupt 2 (tj. NMI) na svuj handler. Ten setne handler nmi window exiting na 1 a injectne NMI do guestu kdyz exitne.

- https://git.back.engineering/_xeroxz/bluepill/src/branch/master/idt_handlers.asm
- https://git.back.engineering/_xeroxz/bluepill/src/branch/master/idt.cpp#L96
- https://git.back.engineering/_xeroxz/bluepill/src/branch/master/exit_handler.cpp#L27
- https://git.back.engineering/_xeroxz/bluepill/src/branch/master/exception.cpp#L34

https://git.back.engineering/_xeroxz/bluepill/#idt-interrupt-descriptor-table
