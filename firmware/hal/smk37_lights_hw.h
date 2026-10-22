/* SPDX-License-Identifier: (GPL-3.0-only OR MPL-2.0) */
#pragma once
#include "smk37_lights.h"
#ifndef SMK37_LIGHT_REG
#define SMK37_LIGHT_REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#endif
#ifndef SMK37_LIGHT_POLL
#define SMK37_LIGHT_POLL() ((void)0)
#endif
#ifndef SMK37_LIGHT_RESET
#define SMK37_LIGHT_RESET() ((void)0)
#endif
#ifndef SMK37_LIGHT_DMA_BARRIER
#define SMK37_LIGHT_DMA_BARRIER() ((void)0)
#endif
/* Main-loop only, between synchronous LCD transfers. IRQs stay enabled.
 * Stock 0x02004fd4/0x0201b412: SPI1 route B, PC10 low/output, baud 8.
 * PC9 is an analog input: do NOT enable its digital clock output. */
static inline int smk37_lights_send(const uint8_t *data, uint32_t limit)
{
    if (!data || !limit || !(SMK37_LIGHT_REG(0x11d00u)&1u)) return 0;
    uint32_t route = SMK37_LIGHT_REG(0x51020u);
    uint32_t baud = SMK37_LIGHT_REG(0x11d04u);
    uint32_t con = SMK37_LIGHT_REG(0x11d00u)&0x3fffu;
    uint32_t out = SMK37_LIGHT_REG(0x50080u)&(1u<<10);
    uint32_t dir = SMK37_LIGHT_REG(0x50088u)&(1u<<10);
    SMK37_LIGHT_REG(0x50080u) |= 1u<<8; /* LCD CS high throughout */
    SMK37_LIGHT_REG(0x11d00u) = 0x4000;
    SMK37_LIGHT_REG(0x51020u) |= 0x10;
    SMK37_LIGHT_REG(0x50080u) &= ~(1u<<10);
    SMK37_LIGHT_REG(0x50088u) &= ~(1u<<10);
    /* Hold the data line low before every frame, including rapid updates. */
    SMK37_LIGHT_RESET();
    SMK37_LIGHT_REG(0x11d04u) = 8;
    SMK37_LIGHT_REG(0x11d00u) = 0x4021;
    SMK37_LIGHT_REG(0x11d0cu) = (uint32_t)(uintptr_t)data;
    SMK37_LIGHT_DMA_BARRIER();
    SMK37_LIGHT_REG(0x11d10u) = SMK37_LIGHT_BYTES;
    int ok = 0;
    while (limit--) {
        SMK37_LIGHT_POLL();
        if (SMK37_LIGHT_REG(0x11d00u)&0x8000u) { ok=1; break; }
    }
    /* Stop DMA even on timeout before restoring the display route. */
    SMK37_LIGHT_REG(0x11d00u) = 0x4000;
    /* Latch the complete frame while the output remains low, before LCD use. */
    SMK37_LIGHT_REG(0x50080u) &= ~(1u<<10);
    SMK37_LIGHT_RESET();
    SMK37_LIGHT_REG(0x50080u) = (SMK37_LIGHT_REG(0x50080u)&~(1u<<10))|out;
    SMK37_LIGHT_REG(0x50088u) = (SMK37_LIGHT_REG(0x50088u)&~(1u<<10))|dir;
    SMK37_LIGHT_REG(0x51020u) = (SMK37_LIGHT_REG(0x51020u)&~0x10u)|(route&0x10u);
    SMK37_LIGHT_REG(0x11d04u) = baud;
    SMK37_LIGHT_REG(0x11d00u) = con|0x4000u;
    return ok;
}
