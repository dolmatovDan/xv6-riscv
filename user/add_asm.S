#include "kernel/syscall.h"

.global main
main:
        addi sp, sp, -48
        sd ra, 40(sp)
        sd s0, 32(sp)
        sd s1, 24(sp)
        sd s2, 16(sp)
        sd s3, 8(sp)

        la s0, buf
        mv a0, s0
        li a1, 100
        call gets

        mv a0, s0
        li a1, 10
        call strchr
        beqz a0, no_newline
        sb zero, 0(a0)
no_newline:

        mv a0, s0
        li a1, 32
        call strchr
        bnez a0, no_error
        la a0, err_msg
        call printf
        li a0, 1
        j done
no_error:
        mv s2, a0

        mv a0, s0
        call atoi
        mv s3, a0

        addi a0, s2, 1
        call atoi
        mv a1, a0
        mv a0, s3
        call add
        mv s3, a0

        la a0, dbg
        mv a1, s0
        call printf

        la a0, res
        mv a1, s3
        call printf

        li a0, 0
done:
        ld ra, 40(sp)
        ld s0, 32(sp)
        ld s1, 24(sp)
        ld s2, 16(sp)
        ld s3, 8(sp)
        addi sp, sp, 48
        ret

.section .data
buf:
        .space 100
dbg:
        .asciz "|%s|\n"
res:
        .asciz "%d\n"
err_msg:
        .asciz "error: no space found\n"
