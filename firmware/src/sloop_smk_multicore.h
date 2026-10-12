/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK lifecycle around the upstream worker. CPU0 keeps sole ownership of
 * controls, sequencing, USB, display and flash. Call checkpoints only between
 * completed render blocks, never while CPU1 owns a voice. */
#pragma once
static void smk37_multicore_prepare(int cold)
{
    fm1_core1_stop(); /* before clearing or copying any application RAM */
#if SLOOP_DUAL_CORE
    if (cold) {
        fm1_core1_bar.magic = FM1_CORE1_BAR_MAGIC;
        fm1_core1_bar.barred = 0;
    }
#else
    (void)cold;
#endif
}
static void smk37_multicore_start(unsigned boot_failures)
{
#if SLOOP_DUAL_CORE
    /* Retry boots and previously retired workers use the serial renderer. */
    if (!boot_failures && !fm1_core1_barred()) {
        if (!fm1_multicore_start()) fm1_core1_bar_set();
    }
    /* Remember online state before the first IRQ can retire the worker. */
    fm1_core1_poll();
#else
    (void)boot_failures;
#endif
}
static void smk37_multicore_checkpoint(void)
{
#if SLOOP_DUAL_CORE
    audio_worker_check();
    fm1_core1_poll();
#endif
}
static void smk37_multicore_stop(int fault)
{
#if SLOOP_DUAL_CORE
    if (fault && audio_worker_online) fm1_core1_bar_set();
    fm1_core1_stop();
    audio_worker_online = 0;
#else
    (void)fault;
    fm1_core1_stop();
#endif
}
