/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../firmware/hal/sloop_smk_storage_map.h"
static uint8_t nor[0x100000],before[0x100000];
static int remaining=-1,writes;
static int sloop_storage_write_allowed(uint32_t,uint32_t);
static int st_read(uint32_t a,void *p,uint32_t n){if(n&&!SLOOP_STORE_RANGE(a,n))return -1;memcpy(p,nor+a,n);return 0;}
static int st_erase(uint32_t a){if((a&4095)||!sloop_storage_write_allowed(a,4096)||!remaining)return -1;
 if(remaining>0){remaining--;}writes++;memset(nor+a,255,4096);return 0;}
static int st_prog(uint32_t a,const void *p,uint32_t n){if(!sloop_storage_write_allowed(a,n)||n>256-(a&255)||!remaining)return -1;
 if(remaining>0){remaining--;}writes++;for(unsigned i=0;i<n;i++)nor[a+i]&=((const uint8_t*)p)[i];return 0;}
#include "../sloop/firmware/src/storage.c"
#include "../firmware/src/sloop_smk_storage_policy.c"
int main(void){
 uint8_t a[3816],b[3816],got[3816];memset(a,0x31,sizeof a);memset(b,0x42,sizeof b);
 memset(nor,255,sizeof nor);sloop_storage_scan();assert(writes==0&&sloop_storage_owned==511);
 for(unsigned o=0;o<OBJ_COUNT;o++)for(unsigned c=0;c<2;c++){
  uint32_t p=st_sector(o,c);assert(SLOOP_STORE_RANGE(p,4096));
  for(unsigned q=0;q<o;q++)for(unsigned d=0;d<2;d++)assert(p!=st_sector(q,d));
 }
 for(unsigned o=0;o<OBJ_COUNT;o++){assert(!st_save(o,a,sizeof a));assert(st_load(o,got,sizeof got)==sizeof a);assert(!memcmp(a,got,sizeof a));}
 memcpy(before,nor,sizeof nor);
 for(unsigned o=0;o<OBJ_COUNT;o++)for(int fail=0;fail<18;fail++){
  memcpy(nor,before,sizeof nor);sloop_storage_scan();remaining=fail;(void)st_save(o,b,sizeof b);remaining=-1;sloop_storage_scan();
  assert(st_load(o,got,sizeof got)==sizeof a);assert(!memcmp(a,got,sizeof a)||!memcmp(b,got,sizeof b));
  for(unsigned q=0;q<OBJ_COUNT;q++)if(q!=o){assert(st_load(q,got,sizeof got)==sizeof a);assert(!memcmp(a,got,sizeof a));}
 }
 /* Every pair rejects unknown data, including a Felucca SMK9 commit. */
 for(unsigned o=0;o<OBJ_COUNT;o++){
  memset(nor,255,sizeof nor);memcpy(nor+st_sector(o,0),"9KMS",4);sloop_storage_scan();memcpy(before,nor,sizeof nor);writes=0;
  assert(st_save(o,a,sizeof a));assert(!writes&&!memcmp(before,nor,sizeof nor));
 }
 memset(nor,255,sizeof nor);sloop_storage_scan();remaining=2;assert(st_save(OBJ_PROJECT0,a,sizeof a));remaining=-1;sloop_storage_scan();writes=0;
 assert(st_save(OBJ_PROJECT0,b,sizeof b));assert(!writes); /* interrupted first save fails closed after reboot */
 assert(!sloop_storage_write_allowed(0xb1000,0));assert(!sloop_storage_write_allowed(0xb8fff,2));
 assert(!sloop_storage_write_allowed(0x4000,4096));assert(!sloop_storage_write_allowed(0xc1000,4096));
 assert(!sloop_storage_write_allowed(0xbf000,4096));assert(!sloop_storage_write_allowed(0x9d000,4096));
 assert(!sloop_storage_write_allowed(0xc2000,4096));assert(!sloop_storage_write_allowed(0xfffffff0,64));
 puts("PASS: nine SMK storage objects, disjoint bounds, save/load, every interrupted commit, foreign/Felucca preservation, failed-first-save refusal, and protected areas.");
}
