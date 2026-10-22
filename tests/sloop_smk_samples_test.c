/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint8_t nor[0x100000],before[0x100000],empty_sample[512],slot_before[0x14000];
static const uint8_t *sloop_sample_xip(unsigned k);
#define SMP_USER_XIP(k) sloop_sample_xip(k)
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){(void)x;(void)y;(void)w;(void)h;(void)p;}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){(void)x;(void)y;(void)w;(void)h;(void)c;}
#include "../firmware/src/sloop_smk_runtime.c"
#include "../firmware/hal/smk37_store.h"
#include "../firmware/hal/smk37_sample_store.h"
#include "../firmware/hal/smk37_public_profile.h"
static uint8_t flash_ok,sloop_samples_owned,sloop_samples_formatting;
static uint32_t sloop_samples_unit=123;
static int writes,remaining=-1;
static int op(void){if(!remaining)return -1;if(remaining>0)--remaining;++writes;return 0;}
static int sloop_storage_read_raw(uint32_t a,void*p,uint32_t n){assert(a<=sizeof nor&&n<=sizeof nor-a);memcpy(p,nor+a,n);return 0;}
static void sloop_storage_silence(void){}
static int fl_erase4k(uint32_t a,uint32_t*t){assert(t);*t=1;if(op())return -1;assert(!(a&4095)&&SMK37_SAMPLE_RANGE(a,4096));memset(nor+a,255,4096);return 0;}
static int fl_write(uint32_t a,const void*p,uint32_t n){
    assert(SMK37_SAMPLE_RANGE(a,n)||SMK37_SAMPLE_JOURNAL_RANGE(a,n));
    if(op())return -1;for(unsigned i=0;i<n;i++)nor[a+i]&=((const uint8_t*)p)[i];return 0;
}
#define FL_XIP(a) ((uintptr_t)(nor+(a)))
#include "../firmware/src/sloop_smk_samples.c"
#define SXQ 2048u
static uint32_t so_w,so_r;
static uint8_t reply[2048];static unsigned reply_n;
static void ota_wire_send(const uint8_t*p,uint32_t n){assert(n<=sizeof reply);memcpy(reply,p,n);reply_n=n;}
static int ota_frame_get(const uint8_t**p,uint32_t*n){(void)p;(void)n;return 0;}
static void ota_frame_done(void){}
static int ed_flash_busy(void){return song.playing||transport_req;}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;}
static int fl_erase4k_quiet(uint32_t a,uint32_t*t){(void)a;(void)t;return -1;}
#define SLOOP_ED_SAMPLES 1
#include "../sloop/firmware/src/editor.c"
static unsigned command(unsigned cmd,const uint8_t*a,unsigned n){
    uint8_t f[650]={125,70,76};f[3]=cmd;memcpy(f+4,a,n);reply_n=0;ed_handle(f,n+4);assert(reply_n>=7);return reply[reply_n-2];
}
static unsigned pack(const uint8_t*p,unsigned n,uint8_t*out){unsigned size=0;for(unsigned i=0;i<n;){unsigned at=size++;out[at]=0;for(unsigned b=0;b<7&&i<n;b++,i++){out[at]|=(p[i]>>7)<<b;out[size++]=p[i]&127;}}return size;}
static void protected(void){for(unsigned i=0;i<sizeof nor;i++)if(!SMK37_SAMPLE_RANGE(i,1)&&!SMK37_SAMPLE_JOURNAL_RANGE(i,1))assert(nor[i]==before[i]);}
int main(void){
    for(unsigned i=0;i<sizeof nor;i++)nor[i]=(i*31u+17u)>>3;
    memset(nor+SMK37_SAMPLE_JOURNAL,255,4096);memcpy(before,nor,sizeof nor);
    sloop_init();flash_ok=1;uint8_t a[640]={0};
    command(ED_SMP_INFO,a,0);assert(reply[5]==0&&reply[6]==0&&!writes);
    assert(command(ED_SMP_BEGIN,a,1)&&!writes);
    song.playing=1;assert(sloop_sample_initialize_request()&&!writes);song.playing=0;
    for(int fail=0;fail<5;fail++){
        memcpy(nor,before,sizeof nor);sloop_samples_owned=0;remaining=fail;
        assert(sloop_sample_initialize_request());remaining=-1;
        assert(!sloop_sample_initialize_request()&&sloop_samples_owned);protected();
    }
    unsigned count=writes;assert(!sloop_sample_initialize_request()&&writes==count);
    command(ED_SMP_INFO,a,0);assert(reply[5]==3&&reply[6]==80);
    sloop_samples_unit=456;count=writes;assert(sloop_sample_initialize_request()&&writes==count);
    sloop_samples_unit=123;assert(!sloop_sample_initialize_request());
    uint8_t data[4097];for(unsigned i=0;i<sizeof data;i++)data[i]=i*7;
    for(unsigned k=0;k<3;k++){
        a[0]=k;assert(!command(ED_SMP_BEGIN,a,1));
        for(unsigned off=0;off<sizeof data;){unsigned n=sizeof data-off;if(n>256)n=256;unsigned addr=512+off;
            a[1]=addr&127;a[2]=(addr>>7)&127;a[3]=addr>>14;
            unsigned packed=pack(data+off,n,a+4);assert(!command(ED_SMP_WRITE,a,4+packed));
            count=writes;assert(!command(ED_SMP_WRITE,a,4+packed)&&writes==count);off+=n;
        }
        smp_user_hdr_t h={0};h.magic=SMP_USER_MAGIC;h.version=1;h.nz=1;memcpy(h.name,"SMK TEST",8);h.data_len=sizeof data;h.crc=st_crc32(data,sizeof data);
        h.zone[0]=(smp_zone_t){.n=8194,.le=8193,.rate=65536,.root16=960,.hi=127};
        unsigned packed=pack((void*)&h,sizeof h,a+1);
        nor[smk37_sample_address(k)+600]^=1;assert(command(ED_SMP_END,a,packed+1)==3);nor[smk37_sample_address(k)+600]^=1;
        memcpy(slot_before,nor+smk37_sample_address(k),sizeof slot_before);
        for(int fail=0;fail<3;fail++){
            remaining=fail;assert(command(ED_SMP_END,a,packed+1)==4);remaining=-1;
            smp_user_scan(k);assert(!usr_nz[k]);
            memcpy(nor+smk37_sample_address(k),slot_before,sizeof slot_before);
        }
        assert(!command(ED_SMP_END,a,packed+1)&&usr_nz[k]==1);
        count=writes;assert(!command(ED_SMP_END,a,packed+1)&&writes==count);
        smp_user_scan(k);assert(usr_nz[k]==1);
        song.playing=1;assert(command(ED_SMP_ERASE,a,1)==7&&writes==count);song.playing=0;
        assert(!command(ED_SMP_ERASE,a,1)&&!usr_nz[k]);protected();
    }
    count=writes;a[0]=3;assert(command(ED_SMP_BEGIN,a,1)&&writes==count);
    a[0]=0;assert(!command(ED_SMP_BEGIN,a,1));count=writes;
    a[1]=0;a[2]=6;a[3]=0;unsigned n=pack(data,256,a+4);assert(command(ED_SMP_WRITE,a,n+4)&&writes==count);
    assert(sloop_sample_erase(0xc1000)&&sloop_sample_program(0xb1000,data,64)&&writes==count);
    smp_user_scan(3);assert(!usr_nz[3]);
    puts("PASS: Sloop physical sample adapter: 3 slots, explicit initialization/recovery, native editor uploads, duplicates, CRC, restart scan, playback refusal and protected regions");
}
