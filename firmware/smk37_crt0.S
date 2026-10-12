/* SPDX-License-Identifier: GPL-3.0-only */
    .section .text.crt0,"ax",@progbits
    .globl _start
_start:
    goto 1f
    rts
1:
    cli
    sp = _ustack_top
    ssp = _sstack_top
    call smk37_cstart
    goto .

    .section .text.smk37_isr_audio,"ax",@progbits
    .globl smk37_isr_audio
smk37_isr_audio:
    [--sp] = {psr, rets, reti}
    [--sp] = {r3-r0}
    call smk37_audio_irq
    {r3-r0} = [sp++]
    {psr, rets, reti} = [sp++]
    csync
    rti

    .section .text.smk37_isr_scanner,"ax",@progbits
    .globl smk37_isr_scanner
smk37_isr_scanner:
    [--sp] = {psr, rets, reti}
    [--sp] = {r3-r0}
    call smk37_scanner_irq
    {r3-r0} = [sp++]
    {psr, rets, reti} = [sp++]
    csync
    rti

#if SMK37_ENABLE_USB_AUDIO
    .section .text.smk37_isr_usb,"ax",@progbits
    .globl smk37_isr_usb
smk37_isr_usb:
    [--sp] = {psr, rets, reti}
    [--sp] = {r3-r0}
    call smk37_usb_irq
    {r3-r0} = [sp++]
    {psr, rets, reti} = [sp++]
    csync
    rti
#endif

/* Same AC79 CPU1 reset entry as upstream crt0.S; separate private stacks. */
#if SLOOP_DUAL_CORE
    .section .text.core1,"ax",@progbits
    .globl fm1_core1_start
fm1_core1_start:
    cli
    icfg = 0
    usp = _cpu1_ustack_top
    sp = _cpu1_sstack_top
    ssp = _cpu1_sstack_top
    reti = fm1_core1_main
    rti
#endif
