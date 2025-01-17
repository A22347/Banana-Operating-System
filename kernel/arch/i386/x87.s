bits 32

global x87Detect
global x87Save
global x87Load
global x87Init
global x87Close

value_37F dw 0x37F
value_37E dw 0x37E
value_37A dw 0x37A

x87Detect:              ;RETURNS A SIZE_T, 0 OR 1
    push ebx
    push ecx
    push edx
    mov eax, 0x1
    cpuid
    test edx, 1<<0
    jz .nox87
    mov eax, 1
    ret
.nox87:
    mov eax, 0
    pop edx
    pop ecx
    pop ebx
    ret

x87Save:
;WARNING: NOTICE WE ARE NOT USING A STACK FRAME FOR THIS, DO NOT CHANGE EBP WITHOUT PUSHING IT FIRST!
    mov eax, [esp + 4]
    fnsave [eax]
    ret

x87Load:        ;TAKES IN A SIZE_T
    mov eax, [esp + 4]
    frstor [eax]
    ret

x87Init:
    mov eax, cr0
    and ax, 0xFFFB        ;clear coprocessor emulation CR0.EM
    or ax, 0x22           ;set coprocessor monitoring  CR0.MP + numeric errors (CRO.NE)
    mov cr0, eax

    fninit

    clts                ;clear task switched bit

    ;allow interrupts for div by zero and invalid operands
    fldcw [value_37A]   ; writes 0x37a, both division by zero and invalid operands cause exceptions.

    ret

x87Close:
    ret
