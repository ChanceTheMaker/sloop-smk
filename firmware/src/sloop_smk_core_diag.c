/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only extension: 7D 'SC1', version 1, op 0. No flash or parameter writes. */
static int sloop_core_diag(const uint8_t *p, uint32_t n) {
    static const uint8_t query[]={0x7d,'S','C','1',1,0};
    if(n!=sizeof query || memcmp(p,query,n)) return 0;
    uint32_t counters[9]={0};
    fm1_irq_off();
#if SLOOP_DUAL_CORE
    counters[0]=audio_worker_online;
    counters[1]=audio_worker.jobs; counters[2]=fm6_pairs;
    counters[3]=audio_worker.timeouts; counters[4]=audio_worker.rejected;
    counters[5]=fm1_core1_fault.count;
    counters[6]=audio_worker.max_job_ticks;
    counters[7]=audio_worker.max_wait_ticks;
    counters[8]=fm1_core1_barred();
#endif
    fm1_irq_on();
    ed_n=0; ed_out[ed_n++]=0xf0;
    for(unsigned i=0;i<sizeof query;i++) ed_b(query[i]);
    for(unsigned i=0;i<9;i++) for(unsigned j=0;j<5;j++) ed_b(counters[i]>>(j*7));
    ed_send(); return 1;
}
