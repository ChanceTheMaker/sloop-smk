#!/usr/bin/env python3
"""Exercise the production visualizer scheduler with a recording LCD backend."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
source = (ROOT / 'sloop/firmware/src/ui_vis.c').read_text(encoding='utf-8')
draw = source.split('static void vis_draw(void)', 1)[1].split('/* SELECT on', 1)[0]
stub = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define VIS_N 12u
#define C_BLACK 0
#define C_WHITE 1
#define C_DIM 2
static const int FONT_S;
static const char *VIS_NAME[12] = {"scope"};
static struct { unsigned force, frame, msg_t; char msg[32]; } ui;
static unsigned vis_shown_last, vis_style, vis_name_t;
static int cv_oy, updates, bands, clears;
static void lcd_fill(int x,int y,int w,int h,int c) { clears++; }
static void vis_update(void) { updates++; }
static void cv_begin(int w,int h,int c) { assert(w==240 && h==120); }
static void cv_blit(int x,int y) { assert(x==0 && y==(bands%2)*120); bands++; }
static void cv_rect(int x,int y,int w,int h,int c) {}
static void cv_text(int x,int y,const int *f,const char *s,int c) {}
static int text_w(const int *f,const char *s) { return 0; }
static void fmt_int(char *s,int n) { sprintf(s,"%d",n); }
static void str_cpy(char *d,const char *s,int n) { snprintf(d,n,"%s",s); }
static unsigned str_len(const char *s) { unsigned n=0; while(s[n]) n++; return n; }
#define STYLE(n) static void n(void) {}
STYLE(vis_scope) STYLE(vis_spectrum) STYLE(vis_spectrogram) STYLE(vis_lissajous)
STYLE(vis_meters) STYLE(vis_ring) STYLE(vis_tape) STYLE(vis_lcd)
STYLE(vis_bounce) STYLE(vis_orbit) STYLE(vis_wires) STYLE(vis_sloop)
'''
checks = r'''
int main(void) {
    vis_name_t=30;
    for(ui.frame=1;ui.frame<=10;ui.frame++) vis_draw();
    assert(updates==10 && bands==20 && clears==0);
    assert(vis_name_t==25); /* Faster trace does not halve title duration. */
    ui.force=1; ui.frame=11; vis_draw();
    assert(updates==11 && bands==22 && clears==0);
    ui.force=0; vis_style=1; vis_name_t=0;
    for(ui.frame=12;ui.frame<=21;ui.frame++) vis_draw();
    assert(updates==16 && bands==32); /* FFT styles retain their budget. */
    ui.force=1; ui.frame=23; vis_draw();
    assert(updates==17 && bands==34 && clears==0);
    puts("PASS: full-rate scope, complete bands without black clear, half-rate other styles");
}
'''
with tempfile.TemporaryDirectory() as directory:
    c = pathlib.Path(directory) / 'scope.c'
    exe = pathlib.Path(directory) / 'scope'
    c.write_text(stub + 'static void vis_draw(void)' + draw + checks, encoding='utf-8')
    subprocess.run(['cc', '-std=c99', '-O1', '-fsanitize=undefined', str(c), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
