/* SPDX-License-Identifier: GPL-3.0-only */
/* Sloop SMK target. Native UI/DSP, SMK peripherals and optional guarded storage. */
#include <stdint.h>
#define SLOOP_SMK_HARDWARE 1
#define FELUCCA_OTA 1
#define FELUCCA_SYSEX 1
#define FELUCCA_UART 0
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#include "smk37_unit.h"
#define FELUCCA_ID SMK37_UPDATE_PRODUCT
#define SLOOP_SMK_VERSION SMK37_UPDATE_VERSION
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_timer.h"
#include "smk37_audio.h"
#include "smk37_lcd_hw.h"
#include "smk37_lights_hw.h"
#include "smk37_panel.h"
#include "smk37_scanner.h"
#include "smk37_contacts.h"
#include "smk37_keyscan.h"
#include "smk37_events.h"
#include "smk37_controls.h"
#include "smk37_button_watch.h"
#include "smk37_rawscan.h"
#include "smk37_adc.h"
#include "smk37_analog_watch.h"
#include "smk37_boot.h"
#include "smk37_bootguard.h"
#include "smk37_time.h"
#include "smk37_memory.h"
#include "smk37_velocity.h"
#include "smk37_pedal.h"
#include "smk37_analog_controls.h"
static void lcd_sync(void);
static void lcd_blit(uint32_t,uint32_t,uint32_t,uint32_t,const uint16_t*);
static void lcd_fill(uint32_t,uint32_t,uint32_t,uint32_t,uint16_t);
#include "sloop_smk_runtime.c"
#include "sloop_smk_editor.c"
#if FELUCCA_FLASH
#include "sloop_smk_storage_diag.c"
#include "sloop_smk_storage_reset.c"
#endif
#if SMK37_ENABLE_TRS
#include "smk37_midi_output.c"
#endif
#include "smk37_loader_image.h"
/* TIMER5 owns USB polling; never enter the SIE again from the editor loop. */
static uint32_t ota_now_ms(void){return fm1_ticks()/24000u;}
static void ota_idle(void){fm1_wdt_feed();}
static void sloop_usb_service(void){
    static const uint8_t query[]={0,0x32,0x45,0,0,0,0x40,0x7f};
    const uint8_t *p;uint32_t n;
    if(ota_frame_get(&p,&n)&&n==sizeof query&&!memcmp(p,query,n)){
        ota_frame_done();ota_wire_send(smk37_update_identity_wire,sizeof smk37_update_identity_wire);return;
    }
#if FELUCCA_FLASH
    if(ota_frame_get(&p,&n) && (sloop_storage_reset(p,n)||sloop_storage_diag(p,n))){ota_frame_done();return;}
#endif
    ed_service();
    /* Unknown vendor frames must not wedge the one-frame receive queue. */
    if(ota_frame_get(&p,&n))ota_frame_done();
}

volatile smk37_bootguard_t smk37_boot_state __attribute__((section(".noinit")));

static int32_t smk37_dma[HALF_FRAMES * 4u] __attribute__((aligned(4),section(".pool")));
#if FELUCCA_FLASH
static void sloop_storage_silence(void){memset(smk37_dma,0,sizeof smk37_dma);}
#endif
static volatile uint32_t smk37_rendered, smk37_late, smk37_scan_frames;
static volatile uint32_t smk37_audio_irqs;
static volatile uint32_t smk37_frame_ticks, smk37_last_velocity;
static uint32_t smk37_frame_last;
static smk37_keyscan_t smk37_keys[37];
static smk37_controls_t smk37_controls;
static volatile smk37_rawscan_t smk37_rawscan;
/* Aligned single-word diagnostic snapshots; no synth mutation in scan IRQ. */
static volatile uint32_t smk37_last_button, smk37_last_encoder;
static volatile smk37_button_watch_t smk37_buttons;
static smk37_adc_t smk37_adc;
static volatile smk37_analog_watch_t smk37_analog_watch;
/* Keep contact duration for measured-time generic velocity curves. Never call
 * synthesis from the scanner ISR or relabel generic response as factory trim. */
static smk37_events_t smk37_key_events;
/* Separate queue: losing an encoder update cannot lose a key release. */
static smk37_events_t smk37_encoder_events;
static smk37_events_t smk37_button_events;
static smk37_events_t smk37_pad_events;
static smk37_pad_state_t smk37_pad_state[16];
static smk37_pedal_t smk37_pedal_input;
static volatile uint32_t smk37_pedal_latest; /* stable level, applied only by audio consumer */
static volatile uint32_t smk37_analog_latest[6]; /* bit 16 valid, low bits raw */
static volatile uint8_t smk37_analog_failed;
static volatile uint32_t smk37_control_overflows;
static volatile uint32_t smk37_key_overflows;
static uint16_t smk37_pedal_gpio_base[4],smk37_pedal_gpio_changed[4];
static uint8_t smk37_pedal_probe_active;
static uint8_t smk37_pedal_frame[16],smk37_pedal_last_contact;
static uint32_t smk37_pedal_contact_changes;
static const uint32_t smk37_pedal_ports[4]={0x50004u,0x50044u,0x50084u,0x501c4u};
static const uint16_t smk37_pedal_exclude[4]={0x07ffu,0x00e0u,0x07c7u,0x0340u};
void smk37_scanner_irq(void)
{
    unsigned key;
    if (!(SMK37_SCAN_CON & 0x8000u)) return;
    unsigned analog_index = smk37_adc.scan.index;
    int analog_result = smk37_adc_spi_done(&smk37_adc);
    if (analog_result < 0) smk37_analog_failed = 1;
    if (analog_result > 0) {
        uint16_t raw = smk37_adc.scan.raw[analog_index];
        smk37_analog_watch_update(&smk37_analog_watch, analog_index,
                                raw);
        if(analog_index==22){
            int pedal=smk37_pedal_sample(&smk37_pedal_input,raw);
            if(pedal>=0)smk37_pedal_latest=(uint32_t)pedal;
        }
        for (unsigned i=0; i<16; ++i) if (analog_index == smk37_pad_input[i]) {
            smk37_pad_change_t change = smk37_pad_sample(&smk37_pad_state[i], raw);
            if (change.velocity) smk37_events_push(&smk37_pad_events, i, change.velocity);
            if (change.release) smk37_events_push(&smk37_pad_events, i, 0);
        }
        for (unsigned i=0; i<4; ++i) if (analog_index==smk37_fader_input[i])
            smk37_analog_latest[i] = 0x10000u | raw;
        if (analog_index==16 || analog_index==17)
            smk37_analog_latest[analog_index-12] = 0x10000u | raw;
    }
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    if (!smk37_scan_tick()) return;
    uint32_t now=fm1_ticks(), elapsed=now-smk37_frame_last;
    smk37_frame_last=now;
    if(smk37_scan_frames && elapsed && elapsed<240000u)
        smk37_frame_ticks=smk37_frame_ticks ? (smk37_frame_ticks*7u+elapsed)/8u : elapsed;
    ++smk37_scan_frames;

    smk37_rawscan_update(&smk37_rawscan, smk37_scan_frame);
    smk37_control_changes_t controls = smk37_controls_tick(&smk37_controls, smk37_scan_frame);
    smk37_button_watch_update(&smk37_buttons, &controls);
    for (unsigned i = 0; i < SMK37_BUTTON_COUNT; ++i) {
        if (controls.pressed & (1u << i)) {
            smk37_last_button = ((i+1u) << 8) | 1u;
            smk37_events_push(&smk37_button_events,i,1);
        }
        if (controls.released & (1u << i)) {smk37_last_button = (i+1u) << 8;smk37_events_push(&smk37_button_events,i,0);}
    }
    for (unsigned i = 0; i < SMK37_ENCODER_COUNT; ++i)
        if (controls.encoder[i]) {
            smk37_last_encoder = ((i+1u) << 8) | (controls.encoder[i] > 0 ? 1u : 2u);
            smk37_events_push(&smk37_encoder_events, i, controls.encoder[i] > 0 ? 1u : 2u);
        }
    for (key = 0; key < 37; ++key) {
        uint32_t duration;
        unsigned raw = smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][0]) |
                       smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][1]) << 1;
        if (!smk37_keyscan_tick(&smk37_keys[key], raw, &duration)) continue;
        (void)smk37_events_push(&smk37_key_events, key, duration);
    }
}

static void smk37_consume_keys(void){
    smk37_event_t e;
    if(smk37_key_events.overflow||smk37_pad_events.overflow||smk37_button_events.overflow){
        fm1_irq_off();smk37_events_discard(&smk37_key_events);smk37_events_discard(&smk37_pad_events);smk37_events_discard(&smk37_button_events);fm1_irq_on();sloop_release();}
    for(unsigned n=0;n<18&&smk37_events_pop(&smk37_button_events,&e);n++)sloop_event(0,e.key,e.duration);
    for(unsigned n=0;n<SMK37_EVENTS&&smk37_events_pop(&smk37_pad_events,&e);n++)sloop_event(3,e.key,e.duration);
    for(unsigned n=0;n<SMK37_EVENTS&&smk37_events_pop(&smk37_key_events,&e);n++)sloop_event(2,e.key,smk37_key_velocity(e.duration,smk37_frame_ticks,0));
    static int8_t phases[8];
    if(smk37_encoder_events.overflow){fm1_irq_off();smk37_events_discard(&smk37_encoder_events);fm1_irq_on();memset(phases,0,sizeof phases);}
    if(smk37_events_pop(&smk37_encoder_events,&e)&&e.key<8){phases[e.key]+=e.duration==1?1:-1;if(phases[e.key]>=4||phases[e.key]<=-4){sloop_event(1,e.key,phases[e.key]>0?1:-1);phases[e.key]=0;}}
    static uint16_t previous[6];static uint8_t valid;
    if(!smk37_analog_failed)for(unsigned i=0;i<6;i++){uint32_t v=smk37_analog_latest[i];if(!(v&0x10000))continue;unsigned raw=v&65535;if(!(valid&(1u<<i))){previous[i]=raw;valid|=1u<<i;continue;}int delta=(int)raw-previous[i];if(delta>-3&&delta<3)continue;previous[i]=raw;if(i<4)sloop_event(4,i,raw);else sloop_event(5,i-4,i==4?smk37_pitch_value(raw):smk37_mod_value(raw));}
    if(sloop_sustain!=(smk37_analog_failed?0:smk37_pedal_latest))sloop_event(6,0,smk37_analog_failed?0:smk37_pedal_latest);
}
static int smk37_panel_command(uint8_t c) { return smk37_lcd_command(c, 4000000u); }
static int smk37_panel_data(const void *p, uint32_t n) { return smk37_lcd_data(p, n, 4000000u); }
static const smk37_panel_io_t smk37_panel_io = {
    smk37_panel_command, smk37_panel_data, fm1_delay_ms
};
static const smk37_audio_hooks_t smk37_audio_hooks = {
    fm1_delay_ms, fm1_irq_attach
};
static uint8_t smk37_display_ok;
static void lcd_sync(void) {} /* SMK transport completes DMA synchronously. */
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *pixels)
{
    if (!smk37_display_ok) return;
    if (!smk37_panel_window(&smk37_panel_io, x, y, w, h) ||
        !smk37_panel_data(pixels, w*h*2u)) smk37_display_ok = 0;
}

static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){uint16_t row[240];if(w>240||x+w>240||y+h>240)return;for(unsigned i=0;i<w;i++)row[i]=swap16(c);for(unsigned i=0;i<h;i++)lcd_blit(x,y+i,w,1,row);}
static uint8_t smk37_light_buffer[SMK37_LIGHT_BYTES] __attribute__((aligned(4)));
static void smk37_light_service(void){
    static uint32_t previous=UINT32_MAX;
    static uint8_t failed;
    if(failed)return;
    static const uint8_t brightness[4]={0,2,4,8};
    fm1_irq_off();
    smk37_scan_indicator_bits=sloop_get_status()[0];
    uint16_t pads=smk37_pad_feedback_mask(&sloop_pad_feedback,fm1_ms);
    unsigned level=brightness[lights_lvl%4],color=2;
    if(smk_pad_light_test){
        pads=smk_pad_light_test==1?0:(uint16_t)(1u<<((smk_pad_light_test-2)%16));
        if(smk_pad_light_test>1){color=2-(smk_pad_light_test-2)/16;if(!level)level=2;}
    }
    fm1_irq_on();
    /* Always show a physical hit when lights are enabled. Do not translate
     * FM-1 piano-key or sequencer lighting into unrelated physical pads. */
    uint32_t state=pads|(level<<16)|(color<<24);
    if(state==previous)return;
    smk37_light_encode_mask(smk37_light_buffer,pads,color,level);
    if(smk37_lights_send(smk37_light_buffer,4000000u))previous=state;
    else {failed=1;smk37_scan_indicator_bits=0;}
}

void smk37_audio_irq(void)
{
    ++smk37_audio_irqs;
    uint8_t pending = smk37_audio_pending();
    smk37_audio_ack_aux(pending);
    if (pending & SMK37_AUDIO_HALF) {
        unsigned half = smk37_audio_free_half(), b, i;
        int32_t *out = smk37_dma + half * HALF_FRAMES * 2u;
        smk37_consume_keys();
        fm6_poll();
#if FELUCCA_UAC
        uac_render_start();
#endif
        for (b = 0; b < HALF_FRAMES; b += CTL) {
            mix_block(out + b * 2u, CTL);
#if FELUCCA_UAC
            uac_tap(out + b * 2u, CTL);
#endif

            for(unsigned j=1;j<CTL;j+=2){unsigned w=scope_w++&(SCOPE_N-1u);scope_buf[w]=out[(b+j)*2u];scope_bufr[w]=out[(b+j)*2u+1];}

        }
        for (i = 0; i < HALF_FRAMES * 2u; ++i) out[i] *= 128;
        smk37_audio_ack_half();
        ++smk37_rendered;
        if (smk37_audio_free_half() != half) ++smk37_late;
    }
}
/* UAC needs one packet each millisecond even while the LCD transfers.
 * A single 2 kHz TIMER5 consumer owns every SIE access. Priority 4 allows it
 * to preempt rendering (3); SMK contact scanning keeps priority 5. The main
 * loop never polls USB concurrently with this consumer. Retry stays in main:
 * usb.up is false throughout its 25 ms reset, so usb_poll touches no SIE. */
#if FELUCCA_UAC
void smk37_usb_irq(void){fm1_timer5_ack();usb_poll();
#if SMK37_ENABLE_TRS
    smk37_trs_poll();
#endif
}
extern void smk37_isr_usb(void);
static void smk37_usb_timer_start(void){
    fm1_timer5_start(smk37_isr_usb,4);
    FM1_T5_PRD=3000u; /* OSC/4 = 6 MHz, 2 kHz */
}
static void smk37_usb_timer_stop(void){FM1_T5_CON=0x4000u;
#if SMK37_ENABLE_TRS
    if(smk37_trs_running)smk37_uart_tx_stop();
#endif
}
#endif
extern void smk37_isr_audio(void);
extern void smk37_isr_scanner(void);

static void fm1_fault(const fm1_crash_t *record)
{
    (void)record;
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();
    smk37_scan_stop();
    smk37_adc_stop(&smk37_adc);
    /* Watchdog reset, without attempting an unverified flash repair. */
    for (;;) ;
}

static void smk37_enter_usb_update(void){
    smk37_bootguard_update(&smk37_boot_state);
    fm1_irq_off();
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();smk37_scan_stop();smk37_adc_stop(&smk37_adc);
    smk37_audio_pc_out(6,0);
    usb_detach();fm1_delay_ms(100);
    volatile uint8_t *dst=(volatile uint8_t *)0x01c0a800u;
    for(unsigned i=0;i<sizeof smk37_loader_image;++i)dst[i]=smk37_loader_image[i];
    __asm__ volatile("csync" ::: "memory");
    void (*volatile enter)(void)=(void (*)(void))0x01c0a800u;
    enter();for(;;);
}

extern uint32_t _bss_start[], _bss_end[], _pool_start[], _pool_end[];
extern uint32_t _data_start[], _data_end[], _data_load[];
extern uint32_t _rt_start[], _rt_end[], _rt_load[];
void smk37_cstart(const uint32_t *boot_parameters)
{
    uint32_t last, remainder = 0, status_at = 0, lights_at = 0;
    uint32_t health_scan = 0, health_audio = 0, health_progress_at = 0;
    uint32_t saved_reset, saved_p33_timeouts;
    uint8_t saved_p3_reset, saved_watchdog;
    /* Shared P33 reset flags: bit 0 is power-on. Snapshot before replacing BSS.
     * Retention and fallback entry are still hardware verification gates. */
    fm1_p33_timeouts = 0; /* P33 helpers run before BSS initialization. */
    fm1_reset_reason();
    saved_reset = fm1_boot.rst_src;
    saved_p3_reset = fm1_boot.p3_rst;
    saved_watchdog = fm1_boot.wdt_con;
    fm1_wdt_arm(0x0d);
    if (smk37_bootguard_begin(&smk37_boot_state, fm1_boot.p3_rst & 1u))
        fm1_enter_uboot();
    smk37_boot_capture(boot_parameters);
    smk37_boot_state.stage = SMK37_BOOT_MEMORY;
    saved_p33_timeouts = fm1_p33_timeouts;
    smk37_memory_zero(_bss_start, _bss_end);
    smk37_memory_zero(_pool_start, _pool_end);
    smk37_memory_copy(_data_start, _data_end, _data_load);
    smk37_memory_copy(_rt_start, _rt_end, _rt_load);
    __asm__ volatile("csync" ::: "memory");
    /* Reset diagnostics live in BSS; preserve the pre-watchdog snapshot. */
    fm1_boot.rst_src = saved_reset;
    fm1_boot.p3_rst = saved_p3_reset;
    fm1_boot.wdt_con = saved_watchdog;
    fm1_p33_timeouts = saved_p33_timeouts;
    smk37_time_init();
    fm1_irq_init();
    fm1_mailbox_clear();
    sloop_init();
    /* Playable startup. Diagnostics remain reachable with the page encoder. */

    smk37_boot_state.stage = SMK37_BOOT_USB;
    usb_start();
    smk37_boot_state.stage = SMK37_BOOT_DISPLAY;
    smk37_lcd_hw_init(4);
    /* Failure disables the display transport; USB/audio can still run. */
    smk37_display_ok = (uint8_t)smk37_panel_init(&smk37_panel_io);
    /* One-shot branding; retain normal startup timing and USB identity. */
    lcd_fill(0,0,240,240,C_BLACK);
    draw_text_box(0, (240 - FONT_L.h) / 2, 240, &FONT_L, "Sloop SMK", C_WHITE, 1);
    draw_text_box(0, 240 - FONT_S.h - 10, 240, &FONT_S, SMK37_RELEASE_VERSION, RGB(156, 156, 156), 1);
    smk37_boot_state.stage = SMK37_BOOT_SCANNER;
    smk37_adc_init(&smk37_adc);
    /* Stock PC10 idle is low/output. Keep it defined between RGB frames. */
    SMK37_LIGHT_REG(0x50080u) &= ~(1u<<10);
    SMK37_LIGHT_REG(0x50088u) &= ~(1u<<10);
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    smk37_scan_init(fm1_irq_attach, smk37_isr_scanner);
    smk37_boot_state.stage = SMK37_BOOT_AUDIO;
    (void)smk37_audio_init(smk37_dma, HALF_FRAMES * 2u,
                          smk37_isr_audio, 3, &smk37_audio_hooks);
    last = fm1_ticks();
#if FELUCCA_UAC
    smk37_usb_timer_start();
#endif
    fm1_irq_enable_all();
    smk37_boot_state.stage = SMK37_BOOT_RUNNING;
#if FELUCCA_FLASH
    /* Probe/read only after interrupts are initialized. No erase or format on boot. */
    persist_boot();panel_init();settings_init();palette_set(settings.palette);autosave_resume();
#endif
    for (;;) {
        uint32_t now = fm1_ticks();
        remainder += now - last;
        last = now;
        while (remainder >= 24000u) { ++fm1_ms; remainder -= 24000u; }
        fm1_wdt_feed();
#if !FELUCCA_UAC
        usb_poll();
#endif
        usb_retry(fm1_ms);
        sloop_usb_service();
#if SMK37_ENABLE_USB_UPDATE
        if(usb.ota_req){usb.ota_req=0;smk37_enter_usb_update();}
#endif

#if SMK37_ENABLE_TRS
        smk37_trs_configure();
#endif

#if SMK37_ENABLE_SAMPLES
        sloop_samples_service();
#endif
        if (smk37_scan_frames != health_scan && smk37_rendered != health_audio) {
            health_scan = smk37_scan_frames;
            health_audio = smk37_rendered;
            health_progress_at = fm1_ms;
        }
        if (smk37_boot_state.pending && fm1_ms >= 30000u &&
            health_scan >= 100u && health_audio >= 100u && fm1_ms - health_progress_at < 1000u)
            smk37_bootguard_healthy(&smk37_boot_state);
        if (fm1_ms - lights_at >= 25u) { lights_at=fm1_ms; smk37_light_service(); }
        if (fm1_ms - status_at >= 40u) {
            status_at = fm1_ms;
            sloop_tick();
        }
        /* Host-requested return uses the mailbox/reset sequence also found
         * in Pro016. It is not a verified forced recovery route. */
        if (usb.uboot_req) {
            smk37_bootguard_update(&smk37_boot_state);
#if FELUCCA_UAC
            smk37_usb_timer_stop();
#endif
            smk37_audio_stop();
            smk37_scan_stop();
            smk37_adc_stop(&smk37_adc);
            usb_detach();
            fm1_delay_ms(30);
            fm1_enter_uboot();
        }
    }
}
