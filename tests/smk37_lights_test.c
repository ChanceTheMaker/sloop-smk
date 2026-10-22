/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t registers[0x52000/4];
static unsigned polls, complete_after;
static void poll(void);
#define SMK37_LIGHT_REG(a) registers[(a)/4]
#define SMK37_LIGHT_POLL() poll()
#include "../firmware/hal/smk37_lights_hw.h"
static void poll(void)
{
    assert(registers[0x50080/4] & 0x100); /* screen never selected */
    assert(registers[0x51020/4] == 0x20010);
    assert(registers[0x11d04/4] == 8);
    assert(registers[0x11d10/4] == SMK37_LIGHT_BYTES);
    assert(!(registers[0x50088/4] & 0x400));
    assert(registers[0x50088/4] & 0x200); /* PC9 analog input preserved */
    assert(!(registers[0x5008c/4] & 0x200));
    if (++polls == complete_after) registers[0x11d00/4] |= 0x8000;
}
static uint32_t decode(const uint8_t *data)
{
    uint32_t word=0;
    for (unsigned i=0; i<24; ++i) {
        assert(data[i]==0xc0 || data[i]==0xf0);
        word=(word<<1)|(data[i]==0xf0);
    }
    return word;
}
int main(void)
{
    uint8_t buffer[SMK37_LIGHT_BYTES];
    unsigned pad_seen=0, indicator_seen[2]={0,0};
    for (unsigned pad=0; pad<16; ++pad) {
        assert(!(pad_seen&(1u<<smk37_pad_light_order[pad])));
        pad_seen |= 1u<<smk37_pad_light_order[pad];
        for (unsigned color=0; color<3; ++color) for (unsigned level=0; level<=255; ++level) {
            smk37_light_encode(buffer,pad,color,level);
            for (unsigned i=0; i<SMK37_LIGHT_RESET_BYTES; ++i) assert(buffer[i]==0);
            uint32_t expected = level>8 ? 8 : level;
            if (color==0) expected<<=8;
            if (color==1) expected<<=16;
            for (unsigned led=0; led<16; ++led)
                assert(decode(buffer+SMK37_LIGHT_RESET_BYTES+24*led)==(led==smk37_pad_light_order[pad] ? expected : 0));
        }
    }
    assert(pad_seen==0xffff);
    for(unsigned mask=0;mask<=0xffffu;++mask) {
        smk37_light_encode_mask(buffer,(uint16_t)mask,2,4);
        for(unsigned p=0;p<16;++p)
            assert(decode(buffer+SMK37_LIGHT_RESET_BYTES+24*smk37_pad_light_order[p])==((mask&(1u<<p)) ? 4u : 0));
    }
    smk37_light_encode(buffer,16,0,8);
    for (unsigned led=0; led<16; ++led) assert(!decode(buffer+SMK37_LIGHT_RESET_BYTES+24*led));
    smk37_light_encode(buffer,0,3,8);
    for (unsigned led=0; led<16; ++led) assert(!decode(buffer+SMK37_LIGHT_RESET_BYTES+24*led));
    for (unsigned i=0; i<19; ++i) {
        unsigned bank=smk37_indicator_order[i]>>4;
        uint32_t bit=~smk37_indicator_mask(i);
        assert(bank<2 && bit && !(bit&(bit-1)));
        assert(!(bit&~0x7f07u)); /* no ADC mux bits or key-select bits */
        assert(!(indicator_seen[bank]&bit)); indicator_seen[bank]|=bit;
    }
    assert(smk37_indicator_mask(19)==UINT32_MAX);
    registers[0x11d00/4]=0x4021;
    registers[0x11d04/4]=4;
    registers[0x51020/4]=0x20000;
    registers[0x50080/4]=0x1400;
    registers[0x50088/4]=0x1600;
    registers[0x5008c/4]=0x100;
    registers[0x51000/4]=0xabcdef; /* USB0 untouched */
    assert(!smk37_lights_send(0,4));
    assert(!smk37_lights_send(buffer,0));
    complete_after=3;
    assert(smk37_lights_send(buffer,4) && polls==3);
    assert(registers[0x11d0c/4]==(uint32_t)(uintptr_t)buffer);
    assert(registers[0x51020/4]==0x20000 && registers[0x11d04/4]==4);
    assert(registers[0x11d00/4]==0x4021);
    assert(registers[0x50080/4]==0x1500 && registers[0x50088/4]==0x1600);
    polls=0; complete_after=0;
    assert(!smk37_lights_send(buffer,7) && polls==7);
    assert(registers[0x51020/4]==0x20000 && registers[0x11d04/4]==4);
    assert(registers[0x11d00/4]==0x4021);
    assert(registers[0x50080/4]==0x1500 && registers[0x50088/4]==0x1600);
    assert(registers[0x51000/4]==0xabcdef);
    puts("SMK lights: waveform, all pad colors/levels, indicator masks, SPI restore and timeout passed");
}
