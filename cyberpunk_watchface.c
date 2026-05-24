#include "lvgl.h"

/* ── Palette ──────────────────────────────────────────────────────────── */
#define CP_BG      0x030A14
#define CP_CYAN    0x00F5FF
#define CP_MAGENTA 0xFF0055
#define CP_YELLOW  0xFFE000
#define CP_GREEN   0x00FF88
#define CP_PURPLE  0x6600CC
#define CP_PINK    0xFF44AA
#define CP_DARK1   0x0D1F3C
#define CP_DARK2   0x06101E
#define CP_WHITE   0xD0EEFF
#define CP_ORANGE  0xFF6600
#define CP_DKRED   0x880033

/* ── Animated handles ─────────────────────────────────────────────────── */
static lv_obj_t *clock_label;
static lv_obj_t *net_label;
static lv_obj_t *scanline;
static lv_obj_t *sec_fill;
static lv_obj_t *pulse_dot;

/* ── Helpers ──────────────────────────────────────────────────────────── */
static lv_obj_t *rect(lv_obj_t *parent, int x, int y, int w, int h, uint32_t col)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(col), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

static lv_obj_t *panel(lv_obj_t *parent, int x, int y, int w, int h,
                        uint32_t bg, uint32_t border_col, int bw)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(border_col), 0);
    lv_obj_set_style_border_width(o, bw, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, int x, int y,
                        const char *txt, const lv_font_t *font, uint32_t col)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, txt);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(col), 0);
    lv_obj_set_pos(o, x, y);
    return o;
}

/* corner bracket helper: draws L-shaped corner at (x,y) with arm length `a` */
static void corner(lv_obj_t *parent, int x, int y, int ax, int ay,
                   int arm, uint32_t col)
{
    /* horizontal arm */
    rect(parent, x, y, arm * ax + (ax < 0 ? 0 : 0), 1, col);
    /* vertical arm */
    rect(parent, x, y, 1, arm * ay + (ay < 0 ? 0 : 0), col);
}

/* ── Timer callback ───────────────────────────────────────────────────── */
static void timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    static uint8_t sec  = 30, min = 47, hour = 22;
    static uint8_t dowi = 5; /* SAT = 5 (0=MON) */
    static bool    blink = false;

    blink = !blink;
    if(++sec >= 60) {
        sec = 0;
        if(++min >= 60) {
            min = 0;
            if(++hour >= 24) { hour = 0; dowi = (dowi + 1) % 7; }
        }
    }

    static char tbuf[8];
    lv_snprintf(tbuf, sizeof(tbuf), "%02d:%02d", hour, min);
    lv_label_set_text(clock_label, tbuf);

    /* scanline sweeps down */
    lv_obj_set_y(scanline, sec % 64);

    /* seconds progress bar */
    lv_obj_set_width(sec_fill, 1 + (sec * 52) / 59);

    /* pulse blink */
    lv_obj_set_style_bg_opa(pulse_dot, blink ? LV_OPA_COVER : LV_OPA_20, 0);

    /* cycling network status every 8 s */
    if(sec % 8 == 0) {
        static uint8_t ni = 0;
        static const char *msgs[] = {
            "JACKED-IN", "ICE: 97%", "CORPO-NET", "SYNC: OK",
            "ACCESS: V", "BREACH OK"
        };
        ni = (ni + 1) % 6;
        lv_label_set_text(net_label, msgs[ni]);
    }
}

/* ══════════════════════════════════════════════════════════════════════
   MAIN CREATE
   Layout (128 × 64):
     y  0- 8  top bar (9 px, magenta)
     y  9     cyan separator
     y 10-37  main zone: clock left | skyline panel right
     y 38-42  seconds progress bar (5 px)
     y 43     yellow separator + ticks
     y 44-53  info row: role label | net status
     y 54-58  HP bar (left) + BAT bar (right)
     y 59-63  footer strip
   ══════════════════════════════════════════════════════════════════════ */
void cyberpunk_watchface_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(CP_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    /* ── TOP BAR ──────────────────────────────────────────────────────── */
    lv_obj_t *top = panel(scr, 0, 0, 128, 9, CP_MAGENTA, CP_MAGENTA, 0);

    rect(top, 0,   0, 5, 9, CP_DKRED);   /* left dark accent */
    rect(top, 123, 0, 5, 9, CP_DKRED);   /* right dark accent */

    /* decorative tick marks inside bar */
    for(int i = 14; i < 115; i += 10)
        rect(top, i, 7, 2, 2, CP_DKRED);

    /* ~110px wide text, centred in 128px bar */
    label(top, 9, 0, "< NIGHT CITY * 2077 >",
          &lv_font_montserrat_10, CP_WHITE);

    /* ── CYAN SEPARATOR ──────────────────────────────────────────────── */
    rect(scr, 0, 9, 128, 1, CP_CYAN);

    /* ── CLOCK ZONE  (y=10-37) ───────────────────────────────────────── */

    /* Corner brackets around clock */
    /* top-left */
    rect(scr, 1, 10, 6, 1, CP_CYAN);
    rect(scr, 1, 10, 1, 6, CP_CYAN);
    /* bottom-left */
    rect(scr, 1, 37, 6, 1, CP_CYAN);
    rect(scr, 1, 32, 1, 6, CP_CYAN);

    /* Clock digit glow backdrop (subtle dark rectangle) */
    rect(scr, 2, 11, 65, 26, CP_DARK2);

    /* Main clock label */
    clock_label = label(scr, 3, 10, "22:47",
                        &lv_font_montserrat_28, CP_CYAN);

    /* Blinking pulse dot */
    pulse_dot = rect(scr, 62, 20, 4, 4, CP_MAGENTA);

    /* Glitch accent stripes beside clock */
    rect(scr, 60, 14, 7, 1, CP_MAGENTA);
    rect(scr, 63, 27, 4, 1, CP_YELLOW);
    rect(scr, 61, 31, 6, 1, CP_PURPLE);

    /* Vertical divider between clock and skyline */
    {
        lv_obj_t *vd = rect(scr, 68, 10, 1, 28, CP_PURPLE);
        lv_obj_set_style_bg_opa(vd, LV_OPA_60, 0);
    }

    /* ── NIGHT CITY SKYLINE PANEL (x=70-126, y=10-37) ───────────────── */
    lv_obj_t *rp = panel(scr, 70, 10, 57, 28, CP_DARK2, CP_MAGENTA, 1);

    /* Background grid dots */
    for(int gy = 3; gy <= 20; gy += 5)
        for(int gx = 3; gx <= 53; gx += 6)
            rect(rp, gx, gy, 1, 1, 0x152640);

    /* Buildings – all share ground at y=23 (top varies, h = 23-top) */
    static const struct { int8_t x, top, w; uint32_t col; } bld[] = {
        { 2, 11, 5, CP_MAGENTA},
        { 8, 15, 3, 0x44006A },
        {12,  5, 4, CP_CYAN  },
        {17, 17, 3, CP_DARK1 },
        {21,  9, 5, CP_PINK  },
        {27, 13, 3, 0x220044 },
        {31,  3, 5, CP_MAGENTA},
        {37, 13, 4, CP_PURPLE},
        {42, 17, 4, CP_CYAN  },
        {47,  9, 5, CP_PINK  },
    };
    for(int i = 0; i < 10; i++)
        rect(rp, bld[i].x, bld[i].top, bld[i].w, 23 - bld[i].top, bld[i].col);

    /* Ground line */
    rect(rp, 1, 23, 54, 1, CP_YELLOW);

    /* Window lights */
    static const struct { int8_t x, y; uint32_t c; } win[] = {
        { 3,13},{ 3,16},{ 3,19},                          /* bld0 */
        {13, 7},{13,10},{13,13},{13,16},{13,19},           /* bld2 tall */
        {22,11},{22,14},{23,17},{22,20},                   /* bld4 */
        {32, 5},{32, 8},{33,11},{32,14},{32,17},{32,20},   /* bld6 tallest */
        {43,19},                                           /* bld8 */
        {48,11},{48,14},{49,17},{48,20},                   /* bld9 */
    };
    static const uint32_t wc[] = {
        CP_YELLOW, CP_CYAN, CP_WHITE, CP_ORANGE, CP_YELLOW, CP_CYAN
    };
    for(int i = 0; i < 23; i++)
        rect(rp, win[i].x, win[i].y, 1, 1, wc[i % 6]);

    /* Rooftop antenna on tallest building */
    rect(rp, 33, 1, 1, 3, CP_MAGENTA);
    rect(rp, 32, 1, 3, 1, CP_MAGENTA);

    /* "NC-NET" label floating above ground */
    label(rp, 2, 13, "NC-NET", &lv_font_montserrat_10, CP_YELLOW);

    /* Top-right corner accent of skyline panel */
    rect(rp, 50, 1, 5, 1, CP_CYAN);
    rect(rp, 54, 1, 1, 4, CP_CYAN);

    /* ── SECONDS PROGRESS BAR (y=38-42) ──────────────────────────────── */
    lv_obj_t *sec_bg = panel(scr, 3, 38, 56, 5, CP_DARK1, CP_CYAN, 1);

    /* tick marks every 10 s */
    for(int i = 0; i < 6; i++)
        rect(sec_bg, 1 + i * 9, 1, 1, 3, 0x1A3A5C);

    sec_fill = rect(sec_bg, 1, 1, 27, 3, CP_CYAN);

    /* "S" indicator dot right of bar */
    rect(scr, 61, 39, 3, 3, CP_CYAN);

    /* ── YELLOW SEPARATOR + TICKS (y=43) ─────────────────────────────── */
    rect(scr, 0, 43, 128, 1, CP_YELLOW);
    for(int i = 4; i < 128; i += 8)
        rect(scr, i, 41, 1, 2, CP_YELLOW);

    /* ── INFO ROW (y=44-53) ───────────────────────────────────────────── */
    /* Role tag with magenta mini-bar accent */
    rect(scr, 2, 45, 2, 8, CP_MAGENTA);
    label(scr, 6, 44, "NTRN V", &lv_font_montserrat_10, CP_MAGENTA);

    /* Separator dot between role and status */
    rect(scr, 52, 48, 2, 2, CP_YELLOW);

    /* Net status – animated */
    net_label = label(scr, 56, 44, "JACKED-IN",
                      &lv_font_montserrat_10, CP_GREEN);

    /* ── STATUS BARS (y=54-58) ────────────────────────────────────────── */

    /* HP bar */
    lv_obj_t *hp_bg = panel(scr, 2, 54, 56, 5, CP_DARK1, CP_GREEN, 1);
    rect(hp_bg, 1, 1, 42, 3, CP_GREEN);   /* ~75% health */
    /* segmentation marks */
    for(int i = 13; i < 54; i += 13)
        rect(hp_bg, i, 0, 1, 5, CP_DARK2);

    /* HP label left of bar */
    rect(scr, 60, 54, 2, 5, CP_GREEN);

    /* BAT bar */
    lv_obj_t *bat_bg = panel(scr, 70, 54, 56, 5, CP_DARK1, CP_YELLOW, 1);
    rect(bat_bg, 1, 1, 35, 3, CP_YELLOW); /* ~63% battery */
    for(int i = 13; i < 54; i += 13)
        rect(bat_bg, i, 0, 1, 5, CP_DARK2);

    rect(scr, 126, 54, 2, 5, CP_YELLOW);  /* BAT right accent */

    /* Labels above bars (tiny, y=53 overlaps into bar area - glitch look) */
    rect(scr, 2,  51, 8, 1, CP_GREEN);    /* "HP" underline accent */
    rect(scr, 70, 51, 8, 1, CP_YELLOW);   /* "BAT" underline accent */

    /* ── FOOTER STRIP (y=59-63) ──────────────────────────────────────── */
    lv_obj_t *footer = panel(scr, 0, 59, 128, 5, CP_DKRED, CP_DKRED, 0);

    /* Arasaka-style vertical stripe pattern */
    for(int i = 6; i < 122; i += 9)
        rect(footer, i, 0, 2, 5, CP_MAGENTA);

    /* Corner squares */
    rect(footer, 0,   0, 5, 5, CP_MAGENTA);
    rect(footer, 123, 0, 5, 5, CP_MAGENTA);

    /* ── SCANLINE OVERLAY (top-most) ──────────────────────────────────── */
    scanline = rect(scr, 0, 0, 128, 1, CP_CYAN);
    lv_obj_set_style_bg_opa(scanline, LV_OPA_20, 0);

    /* ── START TIMER ──────────────────────────────────────────────────── */
    lv_timer_create(timer_cb, 1000, NULL);
}
