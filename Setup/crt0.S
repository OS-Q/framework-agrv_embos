.section .crt0
.global _start
.global main
.weak   irq_callback
.weak   esr_callback

_start:
  j crtInit

crtInit:
  la t0, trap_entry
  csrw mtvec, t0
  csrwi mstatus, 0
  csrwi mie, 0

  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  la sp, _stack_start

bss_init:
  la a0, _bss_start
  la a1, _bss_end
bss_loop:
  beq a0,a1,bss_done
  sw zero,0(a0)
  add a0,a0,4
  j bss_loop
bss_done:

//ctors_init:
//  la a0, _ctors_start
//  addi sp,sp,-4
//ctors_loop:
//  la a1, _ctors_end
//  beq a0,a1,ctors_done
//  lw a3,0(a0)
//  add a0,a0,4
//  sw a0,0(sp)
//  jalr  a3
//  lw a0,0(sp)
//  j ctors_loop
//ctors_done:
//  addi sp,sp,4


//  li a0, 0x880     //880 enable timer + external interrupts
//  csrw mie,a0
//  li a0, 0x1808     //1808 enable interrupts
//  csrw mstatus,a0

  call main
  call _exit
infinitLoop:
  j infinitLoop

irq_callback:
  ret

esr_callback:
  j esr_callback
