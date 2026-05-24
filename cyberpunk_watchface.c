#include "lvgl.h"

static lv_obj_t *clock_label;
static lv_obj_t *scanline;

static void timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    static uint8_t sec = 0;
    static uint8_t min = 47;
    static uint8_t hour = 22;

    sec++;

    if(sec >= 60) {
        sec = 0;
        min++;
    }

    if(min >= 60) {
        min = 0;
        hour++;
    }

    if(hour >= 24) {
        hour = 0;
    }

    static char buf[16];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
    lv_label_set_text(clock_label, buf);

    lv_obj_set_y(scanline, (sec % 64));
}

void cyberpunk_watchface_create(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x050816), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    lv_obj_t *top_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(top_bar, 128, 10);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0xFF0055), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);

    lv_obj_t *city = lv_obj_create(lv_screen_active());
    lv_obj_set_size(city, 50, 24);
    lv_obj_set_pos(city, 74, 14);
    lv_obj_set_style_bg_color(city, lv_color_hex(0x10182A), 0);
    lv_obj_set_style_border_color(city, lv_color_hex(0x00F0FF), 0);
    lv_obj_set_style_border_width(city, 1, 0);
    lv_obj_set_style_radius(city, 0, 0);

    for(int i = 0; i < 6; i++) {
        lv_obj_t *building = lv_obj_create(city);
        lv_obj_set_size(building, 4, 4 + (i * 2));
        lv_obj_set_pos(building, 3 + (i * 7), 20 - (i * 2));
        lv_obj_set_style_bg_color(building, lv_color_hex(0xFF00AA), 0);
        lv_obj_set_style_border_width(building, 0, 0);
        lv_obj_set_style_radius(building, 0, 0);
    }

    clock_label = lv_label_create(lv_screen_active());
    lv_label_set_text(clock_label, "22:47");
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(clock_label, lv_color_hex(0x00F0FF), 0);
    lv_obj_set_pos(clock_label, 4, 14);

    lv_obj_t *subtitle = lv_label_create(lv_screen_active());
    lv_label_set_text(subtitle, "NIGHT CITY NET");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xFFE600), 0);
    lv_obj_set_pos(subtitle, 6, 44);

    lv_obj_t *battery_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(battery_bar, 40, 6);
    lv_obj_set_pos(battery_bar, 84, 52);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x1B2338), 0);
    lv_obj_set_style_border_color(battery_bar, lv_color_hex(0xFF0055), 0);
    lv_obj_set_style_border_width(battery_bar, 1, 0);
    lv_obj_set_style_radius(battery_bar, 0, 0);

    lv_obj_t *battery_fill = lv_obj_create(battery_bar);
    lv_obj_set_size(battery_fill, 30, 4);
    lv_obj_set_pos(battery_fill, 1, 1);
    lv_obj_set_style_bg_color(battery_fill, lv_color_hex(0x00FF99), 0);
    lv_obj_set_style_border_width(battery_fill, 0, 0);
    lv_obj_set_style_radius(battery_fill, 0, 0);

    scanline = lv_obj_create(lv_screen_active());
    lv_obj_set_size(scanline, 128, 1);
    lv_obj_set_pos(scanline, 0, 0);
    lv_obj_set_style_bg_color(scanline, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_bg_opa(scanline, LV_OPA_30, 0);
    lv_obj_set_style_border_width(scanline, 0, 0);
    lv_obj_set_style_radius(scanline, 0, 0);

    lv_timer_create(timer_cb, 1000, NULL);
}
