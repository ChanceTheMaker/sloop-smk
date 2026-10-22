/* SPDX-License-Identifier: GPL-3.0-only */
/* Real SMK runtime + concurrent upstream worker. Run twice (0 and 1) and
 * compare PCM: physical key/pad/pedal/knob routing must keep the same sound. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <sched.h>
#define SLOOP_DUAL_CORE 1
static uint32_t worker_clock(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((uint64_t)t.tv_sec * 1000000u + t.tv_nsec / 1000u);
}
static void hold_worker(void);
#define AW_WORD _Atomic uint32_t
#define AW_LOAD(p) atomic_load_explicit(p, memory_order_acquire)
#define AW_STORE(p,v) atomic_store_explicit(p,v,memory_order_release)
#define AW_TICKS() worker_clock()
#define AW_TIMEOUT_TICKS 2000000u
#define AW_RAM_LOOP static
#define AW_IDLE() pthread_testcancel()
#define AW_HOLD() hold_worker()
#include "../firmware/src/audio_worker.h"
static void hold_worker(void) {
    while (AW_LOAD(&audio_worker.complete) != AW_LOAD(&audio_worker.request)) sched_yield();
}
static int serial_reference;
#define audio_worker_submit(fn,context) (serial_reference ? 0 : audio_worker_submit(fn,context))
#define __attribute__(x)
#define memset sloop_memset
#define memcpy sloop_memcpy
#define memcmp sloop_memcmp
#ifndef SLOOP_TEST_ENGINE
#define SLOOP_TEST_ENGINE "../web/audio/sloop-smk-engine.c"
#endif
#include SLOOP_TEST_ENGINE
#undef memset
#undef memcpy
#undef memcmp
static void *worker(void *unused) { (void)unused; audio_worker_loop(); return 0; }
int main(int argc, char **argv) {
    assert(argc == 3);
    serial_reference = atoi(argv[1]) == 0;
    FILE *pcm = fopen(argv[2], "wb"); assert(pcm);
    synth_init();
    set_engine_of(&trk[0], ENGI_FM6); apply_preset_to(&trk[0], 0);
    set_engine_of(&trk[1], ENGI_FM6); apply_preset_to(&trk[1], 4);
    pthread_t thread;
    assert(!pthread_create(&thread, 0, worker, 0));
    while (!AW_LOAD(&audio_worker.ready)) sched_yield();
    audio_worker_online = 1; /* same voice caps; reference refuses dispatch */
    uint64_t energy = 0;
    for (unsigned block = 0; block < 4096; block++) {
        assert(AW_LOAD(&audio_worker.request) == AW_LOAD(&audio_worker.complete));
        audio_worker_check();
        if (block == 4) { song.sel = 0; for (unsigned k=12;k<24;k+=3) sloop_event(2,k,100); }
        if (block == 80) { song.sel = 1; for (unsigned k=24;k<36;k+=3) sloop_event(2,k,90); }
        if (block == 140) sloop_event(6,0,1);
        if (block == 150) for (unsigned k=12;k<36;k+=3) sloop_event(2,k,0);
        if (block == 180) sloop_event(5,0,4096);
        if (block == 200) sloop_event(5,1,80);
        if (block == 230) sloop_event(3,0,110);
        if (block == 240) sloop_event(3,0,0);
        if (block == 260) sloop_event(4,0,750);
        if (block == 300) sloop_event(6,0,0);
        if (block == 350) sloop_release();
        if (block >= 400 && block % 100 == 0) {
            sloop_release(); song.sel=0;
            set_engine_of(&trk[0], ENGI_FM6); apply_preset_to(&trk[0], (block/100)%8);
            trk[0].p[P_E0]=(block/100)%33;
            for(unsigned k=0;k<6;k++) sloop_event(2,5+k*2,64+k*9);
        }
        if (block==4080) set_engine_of(&trk[0], 0);
        /* 2.8 assigns whole render units to CPU1. Exercise every engine on
         * that assigned part, alongside CPU0 FM6 and the drum unit. */
        if (block>=400 && block%200==0) {
            song.sel=2;
            for(unsigned k=0;k<4;k++) sloop_event(2,k,0);
            unsigned engine=((block-400)/200)%NENGINES;
            set_engine_of(&trk[2],engine);apply_preset_to(&trk[2],0);
            for(unsigned k=0;k<4;k++) sloop_event(2,k,75+k*8);
            sloop_event(3,(block/200)%16,95);
            sloop_event(3,(block/200)%16,0);
        }
        int32_t *samples = synth_render();
        assert(AW_LOAD(&audio_worker.request) == AW_LOAD(&audio_worker.complete));
        assert(fwrite(samples, sizeof *samples, CTL*2, pcm) == CTL*2);
        for (unsigned i=0;i<CTL*2;i++) energy += samples[i] < 0 ? -(int64_t)samples[i] : samples[i];
    }
    assert(energy && !audio_worker.timeouts && !audio_worker.rejected);
    assert(serial_reference ? !audio_worker.jobs : audio_worker.jobs > 0);
    assert(!pthread_cancel(thread)); assert(!pthread_join(thread, 0));
    assert(!fclose(pcm));
    printf("SMK runtime: %s, %u completed jobs, PCM energy %llu\n", serial_reference?"serial":"concurrent", audio_worker.jobs, (unsigned long long)energy);
    return 0;
}

