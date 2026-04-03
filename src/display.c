/*
 * display.c — LVGL display thread (single-core Zephyr)
 *
 * Runs as a normal Zephyr thread at priority 7.
 * Communicates with the Wi-Fi/main task via the global g_shared struct.
 *
 * GUI layout (240×240 ST7789, dark theme):
 *
 *   ┌──────────────────────────┐
 *   │ ▐  WAKE ON LAN      ⠋   │  ← header card  (y=4,  h=44)
 *   ├──────────────────────────┤
 *   │  MY IP                   │  ← ip card      (y=54, h=56)
 *   │  192.168.1.42            │
 *   ┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
 *   │  TARGET PC          ON   │  ← target card  (y=118, h=56)
 *   │  192.168.1.10            │
 *   ┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈
 *   │      ⚡ WoL Sent!        │  ← event bar    (y=182, h=44)
 *   │  ESP32  •  Zephyr RTOS   │
 *   └──────────────────────────┘
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <lvgl.h>
#include <string.h>

#include "shared.h"
#include "display.h"

#define BACKLIGHT_BRIGHTNESS 20

static const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(DT_ALIAS(bl_pwm));

void set_backlight_brightness(uint8_t percent) {
    if (!device_is_ready(backlight.dev)) return;

    if (percent > 100) percent = 100;

    /* Calcula o duty cycle baseado no período definido no DT (5000ns) */
    uint32_t pulse = (backlight.period * percent) / 100;

    pwm_set_dt(&backlight, backlight.period, pulse);
}

LOG_MODULE_REGISTER(display, CONFIG_LOG_DEFAULT_LEVEL);

/* ── Palette Midnight Ultra ── */
#define C_BG        0x000000
#define C_CARD      0x0A0C14
#define C_CARD_LIT  0x141824
#define C_BORDER    0x1E2433
#define C_ACCENT    0x00B0FF
#define C_GREEN     0x00C853
#define C_RED       0xFF1744
#define C_GOLD      0xFFAB00
#define C_WHITE     0xF5F5F7
#define C_MUTED     0x4A5568

static const char * const DOTS[] = { ".", "..", "..." };
#define DOTS_N 3

typedef struct {
    lv_obj_t *hdr_subtitle;
    lv_obj_t *hdr_spin;
    lv_obj_t *lbl_ip_val;
    lv_obj_t *lbl_tgt_ip;
    lv_obj_t *lbl_tgt_state;
    lv_obj_t *wol_bar;
    lv_obj_t *lbl_wol;
} ui_t;

/* ── Helpers ── */
static lv_obj_t *make_card(lv_obj_t *p, int x, int y, int w, int h, bool lit) {
    lv_obj_t *c = lv_obj_create(p);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(c, lv_color_hex(lit ? C_CARD_LIT : C_CARD), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c, 10, 0);
    
    /* CORREÇÃO CRÍTICA: Faz com que os filhos (faixa azul) sejam cortados nos cantos arredondados */
    lv_obj_set_style_clip_corner(c, true, 0);
    
    lv_obj_set_style_pad_all(c, 0, 0);
    return c;
}

static lv_obj_t *make_label(lv_obj_t *p, const lv_font_t *f, uint32_t c, int align, int ox, int oy, const char *txt) {
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(c), 0);
    lv_obj_align(l, align, ox, oy);
    lv_label_set_text(l, txt);
    return l;
}

static void make_accent_strip(lv_obj_t *card, int h) {
    lv_obj_t *s = lv_obj_create(card);
    lv_obj_set_pos(s, 0, 0); // Garante posição na origem do card
    lv_obj_set_size(s, 4, h);
    lv_obj_set_style_bg_color(s, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_radius(s, 0, 0); // A curva vem do "clip_corner" do pai
}

/* ── Screens ── */
static void build_ap_screen(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
    
    lv_obj_t *hdr = make_card(scr, 4, 4, 232, 44, true);
    make_accent_strip(hdr, 44);
    make_label(hdr, &lv_font_montserrat_14, C_ACCENT, LV_ALIGN_LEFT_MID, 14, -8, "ACCESS POINT");
    make_label(hdr, &lv_font_montserrat_14, C_WHITE, LV_ALIGN_LEFT_MID, 14, 8, "Setup Mode");

    lv_obj_t *ssid = make_card(scr, 4, 54, 232, 56, false);
    make_label(ssid, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_LEFT_MID, 12, -11, "SSID");
    make_label(ssid, &lv_font_montserrat_24, C_WHITE, LV_ALIGN_LEFT_MID, 12, 9, "WOL_ESP");

    lv_obj_t *ipc = make_card(scr, 4, 116, 232, 56, false);
    make_label(ipc, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_LEFT_MID, 12, -11, "PORTAL");
    make_label(ipc, &lv_font_montserrat_24, C_ACCENT, LV_ALIGN_LEFT_MID, 12, 9, "192.168.4.1");
}

static void build_station_screen(ui_t *ui) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);

    lv_obj_t *hdr = make_card(scr, 4, 4, 232, 44, true);
    make_accent_strip(hdr, 44);
    make_label(hdr, &lv_font_montserrat_14, C_ACCENT, LV_ALIGN_LEFT_MID, 14, -8, "WAKE ON LAN");

    ui->hdr_subtitle = make_label(hdr, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_LEFT_MID, 14, 8, "Connecting");
    
    ui->hdr_spin = make_label(hdr, &lv_font_montserrat_14, C_ACCENT, LV_ALIGN_RIGHT_MID, -15, 0, LV_SYMBOL_REFRESH);
    lv_obj_set_style_transform_pivot_x(ui->hdr_spin, 8, 0); 
    lv_obj_set_style_transform_pivot_y(ui->hdr_spin, 8, 0);

    lv_obj_t *ipc = make_card(scr, 4, 54, 232, 56, false);
    make_label(ipc, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_LEFT_MID, 12, -11, "MY IP");
    ui->lbl_ip_val = make_label(ipc, &lv_font_montserrat_24, C_WHITE, LV_ALIGN_LEFT_MID, 12, 9, "");

    lv_obj_t *tgt = make_card(scr, 4, 118, 232, 56, false);
    make_label(tgt, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_LEFT_MID, 12, -11, "IP TARGET");
    ui->lbl_tgt_ip = make_label(tgt, &lv_font_montserrat_24, C_WHITE, LV_ALIGN_LEFT_MID, 12, 9, "");
    ui->lbl_tgt_state = make_label(tgt, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_RIGHT_MID, -12, 9, "");

    ui->wol_bar = make_card(scr, 4, 182, 232, 44, false);
    lv_obj_set_style_bg_opa(ui->wol_bar, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(ui->wol_bar, LV_OPA_0, 0);
    ui->lbl_wol = make_label(ui->wol_bar, &lv_font_montserrat_24, C_GOLD, LV_ALIGN_CENTER, 0, 0, "");
}

/* ── Thread ── */
static void display_thread(void *p1, void *p2, void *p3) {
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) return;

    if (!device_is_ready(backlight.dev)) {
        LOG_ERR("PWM Backlight device not ready");
        set_backlight_brightness(0);
    } else {
        set_backlight_brightness(BACKLIGHT_BRIGHTNESS);
    }

    display_blanking_off(disp);
    lv_init();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    make_label(scr, &lv_font_montserrat_24, C_ACCENT, LV_ALIGN_CENTER, 0, -15, "Zephyr RTOS");
    make_label(scr, &lv_font_montserrat_14, C_MUTED, LV_ALIGN_CENTER, 0, 15, "WOL by Sergio.I");

    lv_task_handler();
    k_msleep(3000);

    lv_obj_clean(scr);

    if (SHARED->ap_mode) {
        build_ap_screen();
        while (1) { lv_task_handler(); k_msleep(50); }
    }

    static ui_t ui;
    build_station_screen(&ui);

    uint16_t angle = 0;
    uint8_t dots_idx = 0;
    int64_t event_hide_at = 0;

    while (1) {
        int64_t now = k_uptime_get();

        angle = (angle + 120) % 3600; 
        lv_obj_set_style_transform_angle(ui.hdr_spin, angle, 0);

        dots_idx = (now / 350) % DOTS_N;

        if (SHARED->wol_sent || SHARED->factory_reset) {
            bool is_reset = SHARED->factory_reset;
            SHARED->wol_sent = false;
            
            lv_obj_set_style_bg_opa(ui.wol_bar, LV_OPA_COVER, 0);
            lv_obj_set_style_border_opa(ui.wol_bar, LV_OPA_COVER, 0);
            
            if (is_reset) {
                lv_obj_set_style_border_color(ui.wol_bar, lv_color_hex(C_RED), 0);
                lv_obj_set_style_text_color(ui.lbl_wol, lv_color_hex(C_RED), 0);
                lv_label_set_text(ui.lbl_wol, LV_SYMBOL_TRASH " Factory Reset");
            } else {
                lv_obj_set_style_border_color(ui.wol_bar, lv_color_hex(C_GOLD), 0);
                lv_obj_set_style_text_color(ui.lbl_wol, lv_color_hex(C_GOLD), 0);
                lv_label_set_text(ui.lbl_wol, LV_SYMBOL_CHARGE " WoL Sent!");
            }
            
            event_hide_at = now + 2000;
        } else if (event_hide_at && now >= event_hide_at) {
            lv_obj_set_style_bg_opa(ui.wol_bar, LV_OPA_0, 0);
            lv_obj_set_style_border_opa(ui.wol_bar, LV_OPA_0, 0);
            lv_label_set_text(ui.lbl_wol, "");
            event_hide_at = 0;
        }

        if (!SHARED->has_ip) {
            lv_label_set_text(ui.hdr_subtitle, "Connecting");
            lv_obj_set_style_text_color(ui.hdr_subtitle, lv_color_hex(C_MUTED), 0);
            lv_label_set_text(ui.lbl_ip_val, DOTS[dots_idx]);
            lv_label_set_text(ui.lbl_tgt_ip, DOTS[dots_idx]);
            lv_label_set_text(ui.lbl_tgt_state, "");
        } else {
            lv_label_set_text(ui.hdr_subtitle, "Connected");
            lv_obj_set_style_text_color(ui.hdr_subtitle, lv_color_hex(C_GREEN), 0);
            lv_label_set_text(ui.lbl_ip_val, (char*)SHARED->my_ip);
            lv_label_set_text(ui.lbl_tgt_ip, (char*)SHARED->target_ip);
            lv_label_set_text(ui.lbl_tgt_state, SHARED->last_known_state ? "ON" : "OFF");
            lv_obj_set_style_text_color(ui.lbl_tgt_state, lv_color_hex(SHARED->last_known_state ? C_GREEN : C_RED), 0);
        }

        lv_task_handler();
        k_msleep(30);
    }
}

K_THREAD_DEFINE(display_id, 4096, display_thread, NULL, NULL, NULL, 7, 0, 0);