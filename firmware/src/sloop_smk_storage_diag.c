/* SPDX-License-Identifier: GPL-3.0-only */
/* Private diagnostic namespace: 7D 'SMK', version 1. Read-only, project pairs
 * only. Runs in the editor main loop and borrows its reply buffer. */
static int sloop_storage_diag(const uint8_t *f, uint32_t n)
{
    static const uint8_t prefix[4] = {0x7d, 'S', 'M', 'K'};
    if (n < 4u || memcmp(f, prefix, 4)) return 0;
    if (n < 6u) return 1;
    uint32_t op = f[5], rc = 0, slot = 0, copy = 0, off = 0, count = 0;
    uint8_t data[256];
    if (f[4] != 1u || op > 1u) rc = 1;
    else if (op == 0u) { if (n != 6u) rc = 1; }
    else if (n != 12u) rc = 1;
    else {
        slot = f[6]; copy = f[7]; off = f[8] | (uint32_t)f[9] << 7;
        count = f[10] | (uint32_t)f[11] << 7;
        if (slot >= 4u || copy >= 2u || !count || count > sizeof data ||
            off >= 4096u || count > 4096u - off) rc = 1;
        else if (!flash_ok) rc = 2;
        else if (song.playing || transport_req) rc = 3;
        else if (st_read(st_sector(OBJ_PROJECT0 + slot, copy) + off, data, count)) rc = 4;
    }
    ed_n = 0; ed_out[ed_n++] = 0xf0;
    for (unsigned i = 0; i < sizeof prefix; i++) ed_b(prefix[i]);
    ed_b(1); ed_b(op); ed_b(rc);
    if (!rc && op == 0u) {
        ed_b(1); /* capability bit 0: project-pair reads; no write capability */
        ed_b(4); ed_b(2); ed_b(0); ed_b(32); /* four slots, two 4096-byte copies */
        ed_b(0); ed_b(2); /* maximum 256 bytes per request */
    } else if (!rc) {
        for (unsigned i = 6; i < 12u; i++) ed_b(f[i]);
        uint32_t crc = st_crc32(data, count);
        for (unsigned i = 0; i < 5u; i++) ed_b(crc >> (7u * i));
        ed_pack7(data, count);
    }
    ed_send();
    return 1;
}
