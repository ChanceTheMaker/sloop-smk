/* SPDX-License-Identifier: (GPL-3.0-only OR MPL-2.0) */
#pragma once
#include <stdint.h>

/* Pro016: RGB setter 0x02004fb0, permutation 0x0205e7a0,
 * serializer 0x02029454. 16 GRB words, MSB first; each logical bit
 * becomes 0xf0 (one) or 0xc0 (zero).
 * Sloop uses Doom 5006bd7 framing: a reset within the SPI stream on BOTH sides of the colors.
 * GPIO OUT alone does not establish ownership while SPI is routed to PC10.
 * 512 zero bytes take 614 us at 60 MHz LSB / 9 (384 us at 96 MHz / 9).
 * Keep the reset physically on the wire even when switching from LCD data. */
#define SMK37_LIGHT_RESET_BYTES 512u
#define SMK37_LIGHT_COLOR_BYTES (16u*24u)
#define SMK37_LIGHT_BYTES (2u*SMK37_LIGHT_RESET_BYTES+SMK37_LIGHT_COLOR_BYTES)
#ifndef SMK37_LIGHT_MAX_LEVEL
#define SMK37_LIGHT_MAX_LEVEL 8u /* Conservative default for hardware diagnostics. */
#endif
#if SMK37_LIGHT_MAX_LEVEL > 255
#error "SMK pad brightness must fit an 8-bit channel"
#endif
/* Capture the cap used when these inline encoders are parsed. */
enum { SMK37_LIGHT_COMPILED_MAX_LEVEL = SMK37_LIGHT_MAX_LEVEL };
static const uint8_t smk37_pad_light_order[16] = {
    0,15,14,13,12,11,10,9,1,2,3,4,5,6,7,8
};
/* Pro016 0x020586bc: 19 indicator entries, high nibble common bank,
 * low nibble active-low shift-register bit. Physical names were mapped by
 * the owner in Test07; see docs/smk37/CONTROL_TRACE.md. */
static const uint8_t smk37_indicator_order[19] = {
    12,18,0,16,13,29,2,1,9,8,11,10,17,25,24,26,28,30,27
};
static inline void smk37_light_encode_mask(uint8_t out[SMK37_LIGHT_BYTES],
                                          uint16_t pads, unsigned color, unsigned level)
{
    if (level > SMK37_LIGHT_MAX_LEVEL) level = SMK37_LIGHT_MAX_LEVEL;
    for (unsigned i=0; i<SMK37_LIGHT_BYTES; ++i) out[i] = 0;
    for (unsigned i=SMK37_LIGHT_RESET_BYTES; i<SMK37_LIGHT_RESET_BYTES+SMK37_LIGHT_COLOR_BYTES; ++i) out[i] = 0xc0;
    if (color >= 3) return;
    static const uint8_t shifts[3] = {8,16,0}; /* requested R, G, B */
    uint32_t word = (uint32_t)level << shifts[color];
    for (unsigned pad=0; pad<16; ++pad) if (pads&(1u<<pad)) {
        unsigned start = SMK37_LIGHT_RESET_BYTES + 24u*smk37_pad_light_order[pad];
        for (unsigned bit=0; bit<24; ++bit)
            out[start+bit] = word & (1u << (23-bit)) ? 0xf0 : 0xc0;
    }
}
static inline void smk37_light_encode(uint8_t out[SMK37_LIGHT_BYTES],
                                     unsigned pad, unsigned color, unsigned level)
{
    smk37_light_encode_mask(out, pad<16 ? (uint16_t)(1u<<pad) : 0, color, level);
}
/* Individual RGB colors, retaining the existing per-channel brightness cap. */
static inline void smk37_light_encode_rgb(uint8_t out[SMK37_LIGHT_BYTES],const uint32_t rgb[16])
{
    for(unsigned i=0;i<SMK37_LIGHT_RESET_BYTES;i++)out[i]=0;
    for(unsigned i=SMK37_LIGHT_RESET_BYTES+SMK37_LIGHT_COLOR_BYTES;i<SMK37_LIGHT_BYTES;i++)out[i]=0;
    for(unsigned pad=0;pad<16;pad++){
        unsigned r=(rgb[pad]>>16)&255,g=(rgb[pad]>>8)&255,b=rgb[pad]&255;
        if(r>SMK37_LIGHT_MAX_LEVEL)r=SMK37_LIGHT_MAX_LEVEL;
        if(g>SMK37_LIGHT_MAX_LEVEL)g=SMK37_LIGHT_MAX_LEVEL;
        if(b>SMK37_LIGHT_MAX_LEVEL)b=SMK37_LIGHT_MAX_LEVEL;
        uint32_t word=(g<<16)|(r<<8)|b;unsigned start=SMK37_LIGHT_RESET_BYTES+24*smk37_pad_light_order[pad];
        for(unsigned bit=0;bit<24;bit++)out[start+bit]=word&(1u<<(23-bit))?0xf0:0xc0;
    }
}
static inline uint32_t smk37_indicator_mask(unsigned indicator)
{
    return indicator < 19 ? ~(1u << (smk37_indicator_order[indicator]&15u)) : UINT32_MAX;
}
