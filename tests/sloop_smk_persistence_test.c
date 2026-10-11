/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define SLOOP_STORAGE_TEST_BACKEND "../../tests/sloop_smk_persistence_backend.h"
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){(void)x;(void)y;(void)w;(void)h;(void)p;}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){(void)x;(void)y;(void)w;(void)h;(void)c;}
#include "../firmware/src/sloop_smk_runtime.c"
#define SXQ 2048u
static uint32_t so_w,so_r;
static uint8_t reply[2048];static unsigned reply_n;
static void ota_wire_send(const uint8_t*p,uint32_t n){assert(n<=sizeof reply);memcpy(reply,p,n);reply_n=n;}
static int ota_frame_get(const uint8_t**p,uint32_t*n){(void)p;(void)n;return 0;}
static void ota_frame_done(void){}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;}
static int fl_erase4k_quiet(uint32_t a,uint32_t*t){(void)a;(void)t;return -1;}
static int fl_write(uint32_t a,const void*p,uint32_t n){(void)a;(void)p;(void)n;return -1;}
#include "../sloop/firmware/src/editor.c"
#include "../firmware/src/sloop_smk_storage_diag.c"
static unsigned save_wire(unsigned slot){
 uint8_t f[]={125,70,76,9,1,(uint8_t)slot};reply_n=0;ed_handle(f,sizeof f);
 assert(reply_n==10&&reply[5]==1&&reply[6]==slot);return reply[8];
}
static void reboot(void){sloop_storage_reclaim_base=0;transport_req=0;sec_dirty=0;song_dirty=0;sloop_init();persist_boot();}
int main(void){
 memset(nor,255,sizeof nor);reboot();assert(flash_ok);
 for(unsigned i=0;i<4;i++){
  song.g[G_BPM]=101+i;trk[0].p[P_LEVEL]=70+i;trk[0].step[0].note[0]=60+i;assert(project_save(i)==0);assert(project_used(i));
 }
 reboot();for(unsigned i=0;i<4;i++){assert(project_used(i));transport_req=0;project_load(i);assert(song.g[G_BPM]==101+i);assert(trk[0].p[P_LEVEL]==70+i);assert(trk[0].step[0].note[0]==60+i);}
 transport_req=0;project_t old=proj_slot[0];song.g[G_BPM]=160;remaining=0;
 assert(project_save(0)==3);assert(!memcmp(&old,&proj_slot[0],sizeof old));
 remaining=-1;reboot();project_load(0);assert(song.g[G_BPM]==101);
 memset(nor,255,sizeof nor);reboot();remaining=0;
 assert(save_wire(0)==3&&!project_used(0)&&reply[7]==0);remaining=-1;
 probe_ok=0;reboot();assert(save_wire(0)==2&&!project_used(0)&&reply[7]==0);probe_ok=1;
 memset(nor,255,sizeof nor);memcpy(nor+st_sector(OBJ_PROJECT0,0),"9KMS",4);reboot();
 assert(save_wire(0)==4&&!project_used(0));assert(!strcmp(ui.msg,"NOT SAVED: SLOT LOCKED"));assert(!memcmp(nor+st_sector(OBJ_PROJECT0,0),"9KMS",4));
 assert(!strcmp(project_slot_label(0),"LOCKED"));
 {uint8_t q[]={125,70,76,9,2,0};ed_handle(q,sizeof q);assert(reply[5]==2&&reply[7]==0&&reply[8]==4);}
 assert(!strcmp(project_slot_label(1),"EMPTY"));
 song.playing=1;assert(save_wire(1)==1&&!project_used(1));
 song.playing=0;assert(save_wire(1)==0&&project_used(1)&&reply[7]==1);
 assert(!strcmp(project_slot_label(1),"USED"));
 /* Separate negotiated diagnostic namespace; no write dispatch exists. */
 uint8_t q[]={125,83,77,75,1,0};assert(sloop_storage_diag(q,sizeof q));
 assert(reply_n==16&&reply[7]==0&&reply[8]==1&&reply[9]==4&&reply[10]==2);
 uint8_t r[]={125,83,77,75,1,1,0,0,0,0,0,2};
 uint8_t before[8192];memcpy(before,nor+st_sector(OBJ_PROJECT0,0),sizeof before);
 assert(sloop_storage_diag(r,sizeof r)&&reply[7]==0&&reply_n==313);
 uint8_t decoded[256];assert(ed_unpack7(reply+19,reply_n-20,decoded,sizeof decoded)==256);
 assert(!memcmp(decoded,before,256));
 for(unsigned i=0;i<5;i++)assert(reply[14+i]==((st_crc32(decoded,256)>>(7*i))&127));
 r[6]=4;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[6]=0;
 r[7]=2;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[7]=0;
 r[8]=127;r[9]=31;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);
 r[8]=0;r[9]=32;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[9]=0;
 r[11]=0;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[11]=3;
 assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[11]=2;
 song.playing=1;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==3);song.playing=0;
 flash_ok=0;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==2);flash_ok=1;
 r[5]=2;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);r[5]=1;
 r[4]=2;assert(sloop_storage_diag(r,sizeof r)&&reply[7]==1);
 assert(!sloop_storage_diag((const uint8_t*)"unknown",7));
 assert(!memcmp(before,nor+st_sector(OBJ_PROJECT0,0),sizeof before));
 puts("PASS: native Sloop project capture/save/cold reload, sounds/globals, failed overwrite retains slot, empty/foreign/unavailable/busy saves stay unsaved");
}
