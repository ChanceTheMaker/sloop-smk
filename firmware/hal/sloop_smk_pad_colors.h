/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* All sixteen physical pads play drum lanes, independent of the selected synth.
 * Keep the drum-track RGB565 theme color; physical hit/hold feedback is white. */
static inline void sloop_smk_pad_colors(uint32_t rgb[16],uint16_t hits,
                                      uint16_t drum_color,unsigned level)
{
    if(level>150u)level=150u;
    unsigned r=((drum_color>>11)&31u)*level/31u;
    unsigned g=((drum_color>>5)&63u)*level/63u;
    unsigned b=(drum_color&31u)*level/31u;
    uint32_t base=(r<<16)|(g<<8)|b,white=level*0x010101u;
    for(unsigned p=0;p<16;p++)rgb[p]=(hits&(1u<<p))?white:base;
}
