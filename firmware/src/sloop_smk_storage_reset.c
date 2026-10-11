/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit legacy replacement, separate from read-only diagnostic v1.
 * Only A-C; keeps the valid Felucca copy A until a Sloop tombstone commits in B.
 * Never runs at boot. Caller supplies checksums of its verified raw backup. */
static int sloop_storage_crc_range(uint32_t a,uint32_t n,uint32_t *result)
{
    uint8_t b[128]; uint32_t c=~0u;
    while(n){
        unsigned k=n>sizeof b?sizeof b:n;
        if(st_read(a,b,k))return -1;
        for(unsigned i=0;i<k;i++){
            c^=b[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);
        }
        a+=k;n-=k;
    }
    *result=~c;return 0;
}
static uint32_t sloop_storage_reset_slot(uint32_t slot,uint32_t expected_a,uint32_t expected_b)
{
    if(slot>=3u)return 1; /* D is deliberately outside this repair interface. */
    if(!flash_ok)return 2;
    if(song.playing||transport_req||!audio_quiet())return 3;
    uint32_t obj=OBJ_PROJECT0+slot,a=st_sector(obj,0),b=st_sector(obj,1),crc;
    uint32_t legacy[8];st_hdr_t current;
    if(sloop_storage_crc_range(a,4096,&crc))return 4;
    if(crc!=expected_a)return 5;
    if(st_read(a,legacy,sizeof legacy))return 4;
    unsigned offset=legacy[1]==1u?256u:128u;
    if(legacy[0]!=0x534d4b39u || (legacy[1]!=1u&&legacy[1]!=2u) || legacy[2]!=slot ||
       !legacy[4] || legacy[4]>4096u-offset || legacy[6] || legacy[7]!=st_crc32(legacy,28))return 6;
    if(sloop_storage_crc_range(a+offset,legacy[4],&crc))return 4;
    if(crc!=legacy[5])return 6;
    st_hdr_t empty={ST_MAGIC,(uint16_t)obj,1,1,0,0,{0xffffffffu,0xffffffffu},0};
    empty.hcrc=st_crc32(&empty,sizeof empty-4u);
    int existing=st_current(obj,&current);
    if(existing>=0){
        /* A lost acknowledgement may be checked safely; never erase Sloop data. */
        if(existing!=1 || memcmp(&current,&empty,sizeof empty))return 6;
    }else{
        if(sloop_storage_crc_range(b,4096,&crc))return 4;
        if(crc!=expected_b)return 5;
        sloop_storage_reclaim_base=b;
        int rc=st_erase(b);
        if(!rc)rc=st_prog(b,&empty,sizeof empty);
        sloop_storage_reclaim_base=0;
        if(rc || st_head(obj,1,&current) || memcmp(&current,&empty,sizeof empty) || st_body(obj,1,&current))return 7;
    }
    sloop_storage_owned|=1u<<obj;
    memset(&proj_slot[slot],0,sizeof proj_slot[slot]);
    sec_dirty&=(uint8_t)~(1u<<slot);
    if(live_sec==(int8_t)slot)live_sec=-1;
    ui.force=1;ui_message("SLOT READY FOR SLOOP");
    return 0;
}

static int sloop_storage_reset(const uint8_t *f,uint32_t n)
{
    static const uint8_t prefix[5]={125,'S','M','K',2};
    if(n<5u||memcmp(f,prefix,5))return 0;
    if(n<6u)return 1;
    unsigned op=f[5],rc=1;
    if(op==0u&&n==6u)rc=0;
    else if(op==1u&&n==22u&&!memcmp(f+17,"RESET",5)&&f[11]<16u&&f[16]<16u){
        uint32_t a=0,b=0;
        for(unsigned i=0;i<5;i++){a|=(uint32_t)f[7+i]<<(7*i);b|=(uint32_t)f[12+i]<<(7*i);}
        rc=sloop_storage_reset_slot(f[6],a,b);
    }
    ed_n=0;ed_out[ed_n++]=0xf0;
    for(unsigned i=0;i<sizeof prefix;i++)ed_b(prefix[i]);
    ed_b(op);ed_b(rc);
    if(!rc&&op==0u){ed_b(1);ed_b(3);} /* explicit-reset capability; A-C only */
    if(op==1u&&n>=7u)ed_b(f[6]);
    ed_send();return 1;
}
