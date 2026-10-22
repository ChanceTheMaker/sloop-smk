/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Two adjacent pads per Sloop track color, repeated across each eight-pad row.
 * Colors are visual groups only: all pads still play their original drum lanes. */
static inline void sloop_smk_pad_colors(uint32_t rgb[16],uint16_t hits,
                                      const uint16_t colors[4],unsigned level)
{
    if(level>150u)level=150u;
    for(unsigned p=0;p<16;p++){
        uint16_t color=colors[(p/2u)%4u];
        unsigned r=((color>>11)&31u)*level/31u;
        unsigned g=((color>>5)&63u)*level/63u;
        unsigned b=(color&31u)*level/31u;
        rgb[p]=(hits&(1u<<p))?level*0x010101u:(r<<16)|(g<<8)|b;
    }
}
