/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static uint32_t registers[0x52000/4],now;static unsigned locked;
#define SMK37_UART_REG(a) registers[(a)/4]
#define SMK37_UART_SYNC() ((void)0)
static uint8_t smk37_trs_enabled;static uint32_t smk37_trs_bytes;
static void fm1_irq_off(void){locked=1;}
static void fm1_irq_on(void){locked=0;}
static uint32_t fm1_ticks(void){return now;}
#include "../firmware/src/smk37_midi_output.c"
int main(void){
    smk37_trs_configure();assert(!smk37_trs_running&&!SMK37_UART_REG(0x12100));
    SMK37_UART_REG(0x51028)=0xa5a5a5a5;SMK37_UART_REG(0x50088)=0xffffffff;
    smk37_trs_enabled=1;smk37_trs_configure();assert(!locked&&smk37_trs_running);
    assert(SMK37_UART_REG(0x12108)==383);
    assert(SMK37_UART_REG(0x51028)==((0xa5a5a5a5&~(0xf00000u|0x80u))|0x100000u));
    assert(SMK37_UART_REG(0x50088)==(0xffffffffu&~128u));
    assert(!(SMK37_UART_REG(0x12100)&(4u|8u|16u))); /* no UART interrupts */
    smk37_trs_output(0x643c9009);smk37_trs_poll();assert(smk37_trs_busy&&SMK37_UART_REG(0x12118)==3);
    assert(!memcmp(smk37_trs_dma,(uint8_t[]){0x90,60,100},3));assert(!smk37_trs_bytes);
    uint8_t in_flight[16];memcpy(in_flight,smk37_trs_dma,16);
    smk37_trs_output(0x003c8008);smk37_trs_poll();assert(!memcmp(in_flight,smk37_trs_dma,16));
    SMK37_UART_REG(0x12100)|=0x8000;smk37_trs_poll();assert(smk37_trs_bytes==3&&smk37_trs_dma[0]==0x80);
    SMK37_UART_REG(0x12100)&=~0x8000u;smk37_trs_enabled=0;smk37_trs_configure();assert(smk37_trs_stopping&&smk37_trs_running);
    unsigned pending=smk37_trs_queue.write;smk37_trs_output(0x643d9009);assert(smk37_trs_queue.write==pending);
    for(unsigned i=0;i<15;++i){SMK37_UART_REG(0x12100)|=0x8000;smk37_trs_poll();}
    assert(!smk37_trs_running&&smk37_trs_bytes==150); /* 6 note bytes + 16*9 panic bytes */
    /* Disabling with a genuinely held note must send an explicit Note Off
     * before shutting UART down, even if the receiver ignores channel-mode CCs. */
    SMK37_UART_REG(0x12100)&=~0x8000u;
    smk37_trs_enabled=1;smk37_trs_configure();smk37_trs_output(0x64409209);smk37_trs_poll();
    smk37_trs_enabled=0;smk37_trs_configure();
    uint8_t shutdown[256];unsigned count=0;
    for(unsigned i=0;i<20 && smk37_trs_running;i++){
        SMK37_UART_REG(0x12100)|=0x8000;smk37_trs_poll();
        if(smk37_trs_busy){assert(count+smk37_trs_dma_count<=sizeof shutdown);memcpy(shutdown+count,smk37_trs_dma,smk37_trs_dma_count);count+=smk37_trs_dma_count;}
    }
    assert(!smk37_trs_running&&count==147&&!memcmp(shutdown+144,(uint8_t[]){0x82,64,0},3));
    SMK37_UART_REG(0x12100)&=~0x8000u;
    smk37_trs_enabled=1;smk37_trs_configure();smk37_trs_output(0x643c9009);smk37_trs_poll();
    now=24000001;smk37_trs_poll();assert(!smk37_trs_running&&!smk37_trs_enabled&&smk37_trs_faults==1);
    puts("SMK TRS: OFF startup, stock mux masks, DMA ownership/completion, disable panic and stalled-UART timeout passed");
}
