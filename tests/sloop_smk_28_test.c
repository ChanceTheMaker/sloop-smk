/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual SMK adapter, not only upstream's FM-1 panel double. */
#include <assert.h>
#include <stdio.h>
#include "../web/audio/sloop-smk-engine.c"
static void tick(void) { fm1_ms+=40; sloop_tick(); }
int main(void)
{
    synth_init();
    assert(lights_lvl==LIGHTS_LOW);
    lights_from_word(0); assert(lights_lvl==LIGHTS_OFF); /* Explicit saved OFF remains valid. */
    for (unsigned trs=0;trs<2;trs++) for (unsigned off=0;off<2;off++) {
        smk37_trs_enabled=trs; cpu1_off=off; lights_lvl=LIGHTS_HIGH;
        uint32_t w=lights_word();
        smk37_trs_enabled=cpu1_off=lights_lvl=0; lights_from_word(w);
        assert(smk37_trs_enabled==trs && cpu1_off==off && lights_lvl==LIGHTS_HIGH);
    }
    lights_from_word(1u<<26); /* Existing 2.6 SMK: TRS on, CPU1 still defaults on. */
    assert(smk37_trs_enabled && !cpu1_off);
    uint8_t master[]={11,0}; ed_live(master,sizeof master); tick();
    assert(ed_master && !song.master_q12);
    sloop_event(1,0,2); assert(!ed_master && song.master_q12==128);
    master[1]=126; ed_live(master,sizeof master); tick(); assert(song.master_q12>4000);
    master[1]=127; ed_live(master,sizeof master); tick(); assert(song.master_q12==128);

    /* Remote taps cannot swallow a simultaneously held physical button. */
    song.g[G_RMCH]=16;
    remote_cc(B_PLAY,127); remote_cc(B_PLAY,0);
    tick();tick();assert(transport_req==1 || song.playing);
    synth_render();tick();tick();assert(!(fm1_in.buttons&(1u<<panel.btn[B_PLAY])));
    transport_req=2;synth_render();
    sloop_event(0,14,1); /* physical FX */
    remote_cc(B_FX,127);tick();tick();remote_cc(B_FX,0);tick();tick();
    assert(fm1_in.buttons&(1u<<panel.btn[B_FX]));
    sloop_event(0,14,0);tick();assert(!(fm1_in.buttons&(1u<<panel.btn[B_FX])));

    song.playing=transport_req=0;
    midi_set[6]=13;midi_set[7]=1;midi_set[8]=16;midi_set[9]=1;
    step_adv_off=drum_preview=rec_quant_off=1;fxfade=9;flt_reso=7;
    fav[0]=0;fav[1]=FAV_NONE;
    fw_snapshot();
    midi_set[6]=0;step_adv_off=drum_preview=rec_quant_off=fxfade=flt_reso=0;
    assert(!fw_restore());
    assert(midi_set[6]==13 && midi_set[7]==1 && midi_set[8]==16 && midi_set[9]==1);
    assert(step_adv_off && drum_preview && rec_quant_off && fxfade==9 && flt_reso==7);
    assert(fav[0]==0 && fav[1]==FAV_NONE);
    snapshot.crc^=1;assert(fw_restore()==-1);assert(fxfade==9);
    puts("PASS: 2.6 TRS settings, independent CPU1 setting, LIVE/K1 volume, remote/physical button ownership and 2.8 session settings");
}
