/* SPDX-License-Identifier: GPL-3.0-only */
/* Execute the SMK lifecycle policy with simulated reset/worker outcomes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static unsigned holds;
static void fm1_core1_stop(void) { ++holds; }
#if SLOOP_DUAL_CORE
#define FM1_CORE1_BAR_MAGIC 0x52414231u
static struct { uint32_t magic, barred; } fm1_core1_bar;
static unsigned launches, succeeds = 1, faults, was_online;
static int audio_worker_online;
static int fm1_core1_barred(void) { return fm1_core1_bar.magic == FM1_CORE1_BAR_MAGIC && fm1_core1_bar.barred == 1; }
static void fm1_core1_bar_set(void) { fm1_core1_bar.magic = FM1_CORE1_BAR_MAGIC; fm1_core1_bar.barred = 1; }
static int fm1_multicore_start(void) { ++launches; return audio_worker_online = succeeds; }
static void audio_worker_check(void) { if (faults) { fm1_core1_stop(); audio_worker_online = 0; } }
static void fm1_core1_poll(void) {
    if (was_online && !audio_worker_online) fm1_core1_bar_set();
    was_online = audio_worker_online;
}
#endif
#include "../firmware/src/sloop_smk_multicore.h"
int main(void)
{
    smk37_multicore_prepare(1);
    assert(holds == 1);
#if SLOOP_DUAL_CORE
    smk37_multicore_start(1); /* watchdog retry must not launch CPU1 */
    assert(!launches);
    smk37_multicore_start(0);
    assert(launches == 1 && audio_worker_online && !fm1_core1_barred());
    faults = 1;
    smk37_multicore_checkpoint();
    assert(!audio_worker_online && fm1_core1_barred());
    faults = 0;
    smk37_multicore_prepare(0); /* warm reset retains the fallback */
    smk37_multicore_start(0);
    assert(launches == 1 && !audio_worker_online);
    smk37_multicore_prepare(1); /* cold restart allows a new attempt */
    smk37_multicore_start(0);
    assert(launches == 2 && audio_worker_online);
    smk37_multicore_stop(1); /* CPU0 fault while CPU1 active */
    assert(!audio_worker_online && fm1_core1_barred());
    smk37_multicore_prepare(1);
    succeeds = 0;
    smk37_multicore_start(0); /* handshake failure stays serial */
    assert(launches == 3 && !audio_worker_online && fm1_core1_barred());
    smk37_multicore_prepare(0);
    smk37_multicore_start(0);
    assert(launches == 3);
    smk37_multicore_prepare(1);
    succeeds = 1;
    smk37_multicore_start(0);
    smk37_multicore_stop(0); /* intentional loader entry is not a fault */
    assert(!audio_worker_online && !fm1_core1_barred());
    puts("SMK CPU1 lifecycle: cold/warm boot, retry, faults, handshake and updater stop passed");
#else
    smk37_multicore_start(0);
    smk37_multicore_checkpoint();
    smk37_multicore_stop(0);
    assert(holds == 2);
    puts("SMK serial lifecycle: no worker dependencies");
#endif
    return 0;
}
