/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static uint16_t screen[240*240];
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){if(x+w>240||y+h>240)return;for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++)screen[(y+j)*240+x+i]=p[j*w+i];}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){c=(c>>8)|(c<<8);if(x+w>240||y+h>240)return;for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++)screen[(y+j)*240+x+i]=c;}
#include "../../firmware/src/sloop_smk_runtime.c"
#define SXQ 2048u
static uint32_t so_w,so_r;
static uint8_t reply[2048],request[2048];static unsigned reply_n;
static void ota_wire_send(const uint8_t*p,uint32_t n){if(n<=sizeof reply){memcpy(reply,p,n);reply_n=n;}}
static int ota_frame_get(const uint8_t**p,uint32_t*n){(void)p;(void)n;return 0;}
static void ota_frame_done(void){}
#include "../../firmware/src/sloop_smk_editor.c"
#define API __attribute__((visibility("default")))
static int32_t output[CTL*2];static uint64_t frames;
API void synth_init(void){sloop_init();usb.config=1;flash_ok=1;}
API void synth_midi(unsigned st,unsigned a,unsigned b){sloop_midi(st,a,b);}
API int32_t*synth_render(void){fm1_ms=frames*1000/FS;fm6_poll();mix_block(output,CTL);frames+=CTL;for(unsigned i=1;i<CTL;i+=2){unsigned w=scope_w++&(SCOPE_N-1);scope_buf[w]=output[i*2];scope_bufr[w]=output[i*2+1];}return output;}
API void fw_event(unsigned kind,unsigned id,int value){sloop_event(kind,id,value);ui_input();}
API void fw_release(void){sloop_release();}
API uint16_t*fw_screen(void){sloop_tick();return screen;}
API uint32_t*fw_status(void){return sloop_get_status();}
API uint8_t*fw_request(void){return request;}
API uint8_t*fw_reply(void){return reply;}
API unsigned fw_command(unsigned n){reply_n=0;if(n<4||n>sizeof request||request[0]!=125||request[1]!=70||request[2]!=76)return 0;ed_handle(request,n);return reply_n;}
API unsigned fw_changed(void){reply_n=0;ed_sync();return reply_n;}
typedef struct {uint32_t magic;project_t current,slots[4];__typeof__(up_bank) bank;__typeof__(arrangement) arr;__typeof__(song_keep) order;__typeof__(dsu) drums;uint8_t patch[NPART][128];uint32_t prefs[4];uint16_t learn[ML_N];uint8_t midi[6];uint8_t samples[4][0x14000];uint32_t crc;} session_t;
static session_t session;
API uint8_t*fw_snapshot(void){session.magic=0x534c4f31;proj_capture(&session.current);memcpy(session.slots,proj_slot,sizeof proj_slot);memcpy(session.bank,up_bank,sizeof up_bank);session.arr=arrangement;session.order=song_keep;session.drums=dsu;for(unsigned i=0;i<NPART;i++)fm6_pack(fm6_patch[i],session.patch[i]);session.prefs[0]=settings.palette;session.prefs[1]=settings.lowcut;session.prefs[2]=settings.zoom;session.prefs[3]=lights_word();memcpy(session.learn,ml_tab,sizeof ml_tab);memcpy(session.midi,midi_set,sizeof midi_set);memcpy(session.samples,sample_memory,sizeof sample_memory);session.crc=st_crc32(&session,sizeof session-4);return (uint8_t*)&session;}
API unsigned fw_snapshot_size(void){return sizeof session;}
API int fw_restore(void){if(session.magic!=0x534c4f31||session.crc!=st_crc32(&session,sizeof session-4)||!proj_ok(&session.current))return -1;sloop_release();memcpy(proj_slot,session.slots,sizeof proj_slot);memcpy(up_bank,session.bank,sizeof up_bank);arrangement=session.arr;song_keep=session.order;dsu=session.drums;proj_apply(&session.current,1);for(unsigned i=0;i<NPART;i++){uint8_t v[FP_SIZE+1];fm6_unpack(session.patch[i],v);fm6_set_patch(i,v);}settings.palette=session.prefs[0]%NPALETTES;settings.lowcut=!!session.prefs[1];settings.zoom=!!session.prefs[2];lights_from_word(session.prefs[3]);for(unsigned k=0;k<ML_N;k++){unsigned v=session.learn[k];ml_tab[k]=(v>>9)&&ml_free_cc(v&127u)&&ml_id(v>>9)<P_COUNT?v:0;}
for(unsigned k=0;k<4;k++)midi_set[k]=session.midi[k]<=16?session.midi[k]:k==3?10:k+1;midi_set[4]=!!session.midi[4];midi_set[5]=clamp(session.midi[5],1,12);
song.g[G_CH1]=midi_set[0];song.g[G_CH2]=midi_set[1];song.g[G_CH3]=midi_set[2];song.g[G_DRCH]=midi_set[3];song.g[G_CHOTH]=midi_set[4];song.g[G_BEND]=midi_set[5];memcpy(sample_memory,session.samples,sizeof sample_memory);for(unsigned i=0;i<4;i++)smp_user_scan(i);ui.force=1;return 0;}
