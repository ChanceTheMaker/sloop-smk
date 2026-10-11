/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/sloop_smk_storage_map.h"
static uint8_t nor[0x100000],flash_ok;
static int remaining=-1,probe_ok=1,partial_program=-1;
static int sloop_storage_write_allowed(uint32_t,uint32_t);
static int st_read(uint32_t a,void*p,uint32_t n){if(n&&!SLOOP_STORE_RANGE(a,n))return -1;memcpy(p,nor+a,n);return 0;}
static int st_erase(uint32_t a){if(!flash_ok||!sloop_storage_write_allowed(a,4096)||!remaining)return -1;if(remaining>0)--remaining;memset(nor+a,255,4096);return 0;}
static int st_prog(uint32_t a,const void*p,uint32_t n){if(!flash_ok||!sloop_storage_write_allowed(a,n)||!remaining)return -1;if(remaining>0)--remaining;unsigned k=partial_program<0?n:(unsigned)partial_program; if(k>n)k=n;for(unsigned i=0;i<k;i++)nor[a+i]&=((const uint8_t*)p)[i];return k==n?0:-1;}
#include "../sloop/firmware/src/storage.c"
#include "../firmware/src/sloop_smk_storage_policy.c"
static int test_probe(void){sloop_storage_scan();return probe_ok;}
#define SLOOP_STORAGE_PROBE() test_probe()
#define FL_XIP(a) ((uintptr_t)(nor+(a)))
