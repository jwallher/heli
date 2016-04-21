.global uppercase
uppercase:
    sub sp, sp, #12
    str r4, [sp, #0]
    str r5, [sp, #4]
    str lr, [sp, #8]
    mov r5, #0
    mov r4, r0
.top:
    ldrb r1, [r4, r5]
    cmp r1, #0
    beq .done
    b .toupper
.done:
    ldr r4, [sp, #0]
    ldr r5, [sp, #4]
    ldr lr, [sp, #8]
    add sp, sp, #12
    mov pc, lr
.toupper:
    mov r0, r1
    bl toupper
    strb r0, [r4, r5]

    add r5, r5, #1
    b .top

