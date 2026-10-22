/* SPDX-License-Identifier: GPL-3.0-only */
/* Transport regression adapted from Doom 5006bd7. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t regs[0x52000/4];
static unsigned resets,barriers,polls,complete_after;
static void reset_gap(void){
    assert(!(regs[0x11d00/4]&1));
    assert(!(regs[0x50080/4]&(1u<<10))&&!(regs[0x50088/4]&(1u<<10)));
    assert(regs[0x50080/4]&(1u<<8));
    resets++;
}
static void barrier(void){assert(resets==1);barriers++;}
static void poll(void){
    assert(resets==1&&barriers==1);
    assert(regs[0x51020/4]==0x20010&&regs[0x11d04/4]==8&&regs[0x11d10/4]==1408);
    assert(regs[0x50088/4]&(1u<<9));
    if(++polls==complete_after)regs[0x11d00/4]|=0x8000;
}
#define SMK37_LIGHT_MAX_LEVEL 150u
#define SMK37_LIGHT_REG(a) regs[(a)/4]
#define SMK37_LIGHT_RESET() reset_gap()
#define SMK37_LIGHT_DMA_BARRIER() barrier()
#define SMK37_LIGHT_POLL() poll()
#include "../firmware/hal/smk37_lights_hw.h"
#include "../firmware/hal/sloop_smk_pad_colors.h"
#include "../firmware/hal/smk37_pad_feedback.h"
static unsigned decode(const uint8_t *p){unsigned v=0;for(unsigned i=0;i<24;i++){assert(p[i]==0xc0||p[i]==0xf0);v=(v<<1)|(p[i]==0xf0);}return v;}
int main(void){
    uint8_t data[SMK37_LIGHT_BYTES];uint32_t rgb[16];
    for(unsigned mask=0;mask<=0xffffu;mask++){
        sloop_smk_pad_colors(rgb,mask,0xfb03,96);
        smk37_light_encode_rgb(data,rgb);
        for(unsigned i=0;i<SMK37_LIGHT_RESET_BYTES;i++)assert(!data[i]);
        for(unsigned i=SMK37_LIGHT_RESET_BYTES+SMK37_LIGHT_COLOR_BYTES;i<SMK37_LIGHT_BYTES;i++)assert(!data[i]);
        for(unsigned p=0;p<16;p++){
            assert(rgb[p]==((mask&(1u<<p))?0x606060u:0x602409u));
            assert(decode(data+SMK37_LIGHT_RESET_BYTES+24*smk37_pad_light_order[p])==((mask&(1u<<p))?0x606060u:0x246009u));
        }
    }
    sloop_smk_pad_colors(rgb,0x5555,0xffff,0);
    for(unsigned p=0;p<16;p++)assert(!rgb[p]);
    sloop_smk_pad_colors(rgb,1,0x001f,255);
    assert(rgb[0]==0x969696&&rgb[1]==150); /* capped white, custom blue drum theme */
    smk37_pad_feedback_t hit={0};
    smk37_pad_feedback_event(&hit,3,100,1000);smk37_pad_feedback_event(&hit,3,0,1000);
    sloop_smk_pad_colors(rgb,smk37_pad_feedback_mask(&hit,1119),0xfb03,96);
    assert(rgb[3]==0x606060&&rgb[2]==0x602409&&rgb[4]==0x602409);
    sloop_smk_pad_colors(rgb,smk37_pad_feedback_mask(&hit,1120),0xfb03,96);
    assert(rgb[3]==0x602409);
    for(unsigned failed=0;failed<2;failed++){
        regs[0x11d00/4]=0x4021;regs[0x11d04/4]=4;regs[0x51020/4]=0x20000;
        regs[0x50080/4]=0x1400;regs[0x50088/4]=0x1600;regs[0x51000/4]=0xabcdef;
        resets=barriers=polls=0;complete_after=failed?0:3;
        assert(smk37_lights_send(data,7)==!failed);
        assert(resets==2&&barriers==1&&polls==(failed?7:3));
        assert(regs[0x11d00/4]==0x4021&&regs[0x11d04/4]==4&&regs[0x51020/4]==0x20000);
        assert(regs[0x50080/4]==0x1500&&regs[0x50088/4]==0x1600&&regs[0x51000/4]==0xabcdef);
    }
    puts("PASS: all held-pad combinations, white hit/theme colors, reset framing, DMA publication, LCD restore and timeout cleanup");
}
