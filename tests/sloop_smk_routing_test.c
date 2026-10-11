/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
static uint32_t sent[256],count;
static void capture(uint32_t p){assert(count<256);sent[count++]=p;}
#define MIDI_OUTPUT_HOOK(p) capture(p)
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){(void)x;(void)y;(void)w;(void)h;(void)p;}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){(void)x;(void)y;(void)w;(void)h;(void)c;}
#include "../firmware/src/sloop_smk_runtime.c"
int main(void){
 sloop_init();count=0;song.sel=0;
 sloop_event(5,0,8191);assert(count==1&&sent[0]==0x7f7fe00e);
 sloop_event(5,1,127);assert(count==2&&sent[1]==0x7f01b00b);
 song.g[G_CH1]=0;count=0;sloop_event(5,0,0);sloop_event(5,1,20);assert(!count);
 sloop_event(6,0,1);assert(count==2);assert(sent[0]==0x7f40b10b&&sent[1]==0x7f40b20b);
 count=0;sloop_release();assert(count==4&&sent[0]==0x0040b10b&&sent[1]==0x4000e10e);
 song.g[G_CH1]=1;count=0;sloop_event(2,0,100);assert(count==1&&((sent[0]>>8)&255)==0x90);
 song.g[G_CH1]=4;sloop_event(2,0,0);assert(count==2&&((sent[1]>>8)&255)==0x80);
 smk37_trs_enabled=1;uint32_t saved=lights_word();smk37_trs_enabled=0;lights_from_word(saved);assert(smk37_trs_enabled==1);
 lights_from_word(saved&~(1u<<26));assert(!smk37_trs_enabled);
 puts("PASS: Sloop expression channels, MIDI OFF, sustain/reset output, note-off original channel and persisted TRS bit");
}
