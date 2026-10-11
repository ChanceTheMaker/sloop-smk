/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_pad_feedback.h"
int main(void){
    smk37_pad_feedback_t s={0};
    assert(!smk37_pad_feedback_mask(&s,0));
    for(unsigned i=0;i<16;i++){
        uint32_t t=500*i;
        smk37_pad_feedback_event(&s,i,100,t);
        smk37_pad_feedback_event(&s,i,0,t);
        assert(smk37_pad_feedback_mask(&s,t)==(1u<<i));
        assert(smk37_pad_feedback_mask(&s,t+119)==(1u<<i));
        assert(!smk37_pad_feedback_mask(&s,t+120));
    }
    smk37_pad_feedback_event(&s,2,100,9000);
    smk37_pad_feedback_event(&s,7,100,9000);
    assert(smk37_pad_feedback_mask(&s,10000)==((1u<<2)|(1u<<7)));
    smk37_pad_feedback_event(&s,2,0,10000);
    assert(smk37_pad_feedback_mask(&s,10000)==(1u<<7));
    smk37_pad_feedback_event(&s,7,0,10000);
    smk37_pad_feedback_event(&s,3,127,UINT32_MAX-50);
    smk37_pad_feedback_event(&s,3,0,UINT32_MAX-50);
    assert(smk37_pad_feedback_mask(&s,49)==(1u<<3));
    assert(!smk37_pad_feedback_mask(&s,69));
    puts("SMK pad feedback: all physical IDs, short taps, holds, chords, expiry and timer wrap passed");
}
