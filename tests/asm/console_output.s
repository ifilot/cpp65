; SPDX-License-Identifier: LGPL-3.0-or-later
; Copyright (C) 2026 Ivo Filot
; Repository: https://github.com/ifilot/cpp65

; Small 65C02 program assembled with ca65.
;
; The emulator maps writes to $F001 to host putchar-like output.
; This program exercises a real assembled control flow:
;   - reset vector entry
;   - subroutine call/return
;   - indexed absolute loads
;   - loop with a conditional branch
;   - memory-mapped console writes

.p02

CONSOLE_OUT = $F001

.segment "CODE"

reset:
    jsr print_message
finished:
    jmp finished

print_message:
    ldx #$00

msg_loop:
    lda message,x
    beq done
    sta CONSOLE_OUT
    inx
    jmp msg_loop

done:
    rts

message:
    .byte "ASM65 says hello from cpp65!", $0A, $00

.segment "VECTORS"
    .word $0000
    .word reset
    .word $0000
