/* SPDX-License-Identifier: GPL-3.0-only */
#define main persistence_main
#include "sloop_smk_persistence_test.c"
#undef main
#include "../firmware/src/sloop_smk_storage_reset.c"
static uint8_t original[sizeof nor];
static uint32_t checksum(unsigned s,unsigned c){return st_crc32(nor+st_sector(OBJ_PROJECT0+s,c),4096);}
static void fixture(void){
 memset(nor,255,sizeof nor);remaining=-1;partial_program=-1;probe_ok=1;reboot();
 song.g[G_BPM]=143;assert(project_save(3)==0);
 for(unsigned s=0;s<3;s++)for(unsigned c=0;c<2;c++){
  uint8_t *p=nor+st_sector(OBJ_PROJECT0+s,c);unsigned off=c?256:128;
  for(unsigned i=0;i<128;i++)p[off+i]=(uint8_t)(s+c+i);
  uint32_t h[]={0x534d4b39,c?1:2,s,8-c,128,st_crc32(p+off,128),0,0};
  h[7]=st_crc32(h,28);memcpy(p,h,sizeof h);
 }
 reboot();memcpy(original,nor,sizeof nor);
}
static void unchanged_except_b(unsigned slot){
 uint32_t b=st_sector(OBJ_PROJECT0+slot,1);
 assert(!memcmp(nor,original,b));assert(!memcmp(nor+b+4096,original+b+4096,sizeof nor-b-4096));
 assert(!sloop_storage_reclaim_base);
}
int main(int argc,char**argv){
 fixture();uint32_t a=checksum(0,0),b=checksum(0,1);
 assert(sloop_storage_reset_slot(3,0,0)==1);
 assert(sloop_storage_reset_slot(0,a^1,b)==5);
 assert(sloop_storage_reset_slot(0,a,b^1)==5);
 song.playing=1;assert(sloop_storage_reset_slot(0,a,b)==3);song.playing=0;
 flash_ok=0;assert(sloop_storage_reset_slot(0,a,b)==2);flash_ok=1;
 assert(!memcmp(nor,original,sizeof nor));
 uint8_t q[]={125,83,77,75,2,0};assert(sloop_storage_reset(q,sizeof q));assert(reply_n==11&&reply[7]==0&&reply[8]==1&&reply[9]==3);
 uint8_t req[22]={125,83,77,75,2,1,0};
 for(unsigned i=0;i<5;i++){req[7+i]=(a>>(7*i))&127;req[12+i]=(b>>(7*i))&127;}
 memcpy(req+17,"RESET",5);req[17]='X';assert(sloop_storage_reset(req,22)&&reply[7]==1);req[17]='R';
 assert(sloop_storage_reset(req,21)&&reply[7]==1);req[6]=3;assert(sloop_storage_reset(req,22)&&reply[7]==1);req[6]=0;
 req[11]=16;assert(sloop_storage_reset(req,22)&&reply[7]==1);req[11]=(a>>28)&15;
 assert(!memcmp(nor,original,sizeof nor));
 assert(sloop_storage_reset(req,22)&&reply[7]==0&&reply[8]==0);unchanged_except_b(0);
 remaining=0;assert(sloop_storage_reset_slot(0,a,b)==0);unchanged_except_b(0);remaining=-1;
 reboot();assert(!project_used(0)&&project_used(3)&&SLOOP_PROJECT_WRITABLE(0));
 song.g[G_BPM]=117;assert(project_save(0)==0);reboot();project_load(0);assert(song.g[G_BPM]==117);
 transport_req=0;assert(sloop_storage_reset_slot(0,checksum(0,0),checksum(0,1))==6);
 assert(!memcmp(nor+st_sector(OBJ_PROJECT0+3,0),original+st_sector(OBJ_PROJECT0+3,0),8192));
 for(int failure=0;failure<2;failure++){
  fixture();a=checksum(0,0);b=checksum(0,1);remaining=failure;
  assert(sloop_storage_reset_slot(0,a,b)==7);unchanged_except_b(0);remaining=-1;reboot();
  assert(!SLOOP_PROJECT_WRITABLE(0)&&project_used(3));
 }
 for(int n=0;n<32;n++){
  fixture();a=checksum(0,0);b=checksum(0,1);partial_program=n;
  assert(sloop_storage_reset_slot(0,a,b)==7);unchanged_except_b(0);partial_program=-1;reboot();
  assert(!SLOOP_PROJECT_WRITABLE(0)&&project_used(3));
 }
 fixture();nor[st_sector(OBJ_PROJECT0,0)+128]^=1;
 assert(sloop_storage_reset_slot(0,checksum(0,0),checksum(0,1))==6);
 /* Optional private, verified hardware archive: never linked into firmware. */
 if(argc==2){
  fixture();for(unsigned s=0;s<4;s++)for(unsigned c=0;c<2;c++){
   char path[1024];snprintf(path,sizeof path,"%s/slot-%u-copy-%u-pass-1.bin",argv[1],s+1,c);
   FILE*f=fopen(path,"rb");assert(f);assert(fread(nor+st_sector(OBJ_PROJECT0+s,c),1,4096,f)==4096);assert(fgetc(f)==EOF);fclose(f);
  }reboot();memcpy(original,nor,sizeof nor);
  for(unsigned s=0;s<3;s++){
   assert(sloop_storage_reset_slot(s,checksum(s,0),checksum(s,1))==0);
   assert(!memcmp(nor+st_sector(OBJ_PROJECT0+s,0),original+st_sector(OBJ_PROJECT0+s,0),4096));
  }
  assert(!memcmp(nor+st_sector(OBJ_PROJECT0+3,0),original+st_sector(OBJ_PROJECT0+3,0),8192));
  reboot();for(unsigned s=0;s<3;s++){assert(!project_used(s));song.g[G_BPM]=110+s;assert(project_save(s)==0);}
  reboot();for(unsigned s=0;s<3;s++){transport_req=0;project_load(s);assert(song.g[G_BPM]==110+s);}
  assert(project_used(3));puts("PASS: actual archived A-C reset, D identical, A-C save/reboot/load");
 }
 puts("PASS: bounded legacy reset, checksums, protocol, D exclusion, all partial header writes, reboot and save");
}
