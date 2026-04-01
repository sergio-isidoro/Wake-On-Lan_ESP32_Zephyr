#include "display.h"
#include "wifi.h"
#include <lvgl.h>

bool display_ap_mode        = false;
bool display_wifi_ready     = false;
bool display_station_ready  = false;
bool has_ip                 = false;

K_MUTEX_DEFINE(display_mutex);
K_SEM_DEFINE(sem_ui_refresh,    0, 1);
K_SEM_DEFINE(sem_display_start, 0, 1);
K_SEM_DEFINE(sem_wol_sent,      0, 1);

/* ── Spinner — ASCII clock hand, 100% Montserrat-safe ──────────────────────
 *  |  /  -  \  |  /  -  \
 * Looks like a rotating needle at 40 FPS.
 */
static const char * const spinner[] = {
    "O", "X", "O", "X", "O", "X", "O", "X"
};
#define SPINNER_FRAMES  ARRAY_SIZE(spinner)

/* ── Dot-pulse under "Connecting" ──────────────────────────────────────────
 * Advances every 8 frames (~5 Hz).
 */
static const char * const dots[] = { "   ", ".  ", ".. ", "..." };
#define DOTS_FRAMES  ARRAY_SIZE(dots)

void display_task_entry(void *p1, void *p2, void *p3)
{
    k_sem_take(&sem_display_start, K_FOREVER);

    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    for (int i = 0; !device_is_ready(disp) && i < 30; i++) {
        k_msleep(100);
    }

    if (!device_is_ready(disp)) {
        printk("[DISPLAY] ERROR: device never became ready\n");
        return;
    }

    display_blanking_off(disp);

    /* ── Static UI build (once) ─────────────────────────────────────────── */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    static lv_style_t sty_white, sty_dim, sty_green, sty_spinner;

    lv_style_init(&sty_spinner);
    lv_style_set_text_color(&sty_spinner, lv_color_hex(0x40C4FF));
    lv_style_set_text_font(&sty_spinner, &lv_font_montserrat_32);

    lv_style_init(&sty_white);
    lv_style_set_text_color(&sty_white, lv_color_white());
    lv_style_set_text_font(&sty_white, &lv_font_montserrat_24);

    lv_style_init(&sty_dim);
    lv_style_set_text_color(&sty_dim, lv_color_hex(0x888888));
    lv_style_set_text_font(&sty_dim, &lv_font_montserrat_14);

    lv_style_init(&sty_green);
    lv_style_set_text_color(&sty_green, lv_color_hex(0x00E676));
    lv_style_set_text_font(&sty_green, &lv_font_montserrat_14);

    lv_obj_t *lbl_spin   = lv_label_create(scr);
    lv_obj_t *lbl_sub    = lv_label_create(scr);
    lv_obj_t *lbl_myip   = lv_label_create(scr);
    lv_obj_t *lbl_target = lv_label_create(scr);
    lv_obj_t *lbl_wol    = lv_label_create(scr);

    lv_obj_add_style(lbl_spin,   &sty_spinner, LV_PART_MAIN);
    lv_obj_add_style(lbl_sub,    &sty_dim,     LV_PART_MAIN);
    lv_obj_add_style(lbl_myip,   &sty_white,   LV_PART_MAIN);
    lv_obj_add_style(lbl_target, &sty_dim,     LV_PART_MAIN);
    lv_obj_add_style(lbl_wol,    &sty_green,   LV_PART_MAIN);

    lv_obj_align(lbl_spin,   LV_ALIGN_TOP_MID, 0,  30);
    lv_obj_align(lbl_sub,    LV_ALIGN_TOP_MID, 0,  80);
    lv_obj_align(lbl_myip,   LV_ALIGN_TOP_MID, 0, 115);
    lv_obj_align(lbl_target, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_align(lbl_wol,    LV_ALIGN_TOP_MID, 0, 190);

    lv_label_set_text_static(lbl_wol, "");

    /* ── AP MODE ────────────────────────────────────────────────────────── */
    if (display_ap_mode) {
        lv_label_set_text_static(lbl_spin,   "WOL");
        lv_label_set_text_static(lbl_sub,    "Access Point");
        lv_label_set_text_static(lbl_myip,   "192.168.4.1");
        lv_label_set_text_static(lbl_target, "Configure WiFi");
        lv_task_handler();
        display_wifi_ready = true;

        while (1) {
            k_sem_take(&sem_ui_refresh, K_FOREVER);
            lv_task_handler();
        }
    }

    /* ── STATION MODE ───────────────────────────────────────────────────── */
    static char prev_myip[INET_ADDRSTRLEN] = "";
    static char prev_target_str[32]        = "";
    static bool prev_has_ip                = true;  /* force first render */

    uint8_t  spin_idx      = 0;
    uint8_t  dots_idx      = 0;
    uint8_t  dots_div      = 0;
    int32_t  wol_hide_tick = 0;

    display_station_ready = true;

    while (1) {
        k_sem_take(&sem_ui_refresh, K_MSEC(25));   /* 40 FPS cap */

        k_mutex_lock(&display_mutex, K_FOREVER);

        /* ── Spinner ── */
        lv_label_set_text_static(lbl_spin, spinner[spin_idx]);
        spin_idx = (spin_idx + 1) % SPINNER_FRAMES;

        /* ── WOL flash ── */
        if (k_sem_take(&sem_wol_sent, K_NO_WAIT) == 0) {
            lv_label_set_text_static(lbl_wol, ">> WoL sent! <<");
            wol_hide_tick = (int32_t)k_uptime_get_32() + 1500;
        } else if (wol_hide_tick &&
                   (int32_t)(k_uptime_get_32() - wol_hide_tick) >= 0) {
            lv_label_set_text_static(lbl_wol, "");
            wol_hide_tick = 0;
        }

        /* ── IP state ── */
        has_ip = (global_ip_str[0] != '\0' &&
                  strcmp(global_ip_str, "0.0.0.0") != 0);

        if (!has_ip) {
            if (prev_has_ip) {
                lv_label_set_text_static(lbl_myip,   "Connecting");
                lv_label_set_text_static(lbl_target, "");
                prev_myip[0]       = '\0';
                prev_target_str[0] = '\0';
                prev_has_ip        = false;
            }

            /* Dot-pulse under "Connecting" */
            if (++dots_div >= 8) {
                dots_div = 0;
                dots_idx = (dots_idx + 1) % DOTS_FRAMES;
                lv_label_set_text_static(lbl_sub, dots[dots_idx]);
            }

        } else {
            if (!prev_has_ip) {
                lv_label_set_text_static(lbl_sub, "");
                prev_has_ip = true;
            }

            /* Device IP */
            if (strcmp(global_ip_str, prev_myip) != 0) {
                lv_label_set_text(lbl_myip, global_ip_str);
                strncpy(prev_myip, global_ip_str, sizeof(prev_myip) - 1);
                prev_myip[sizeof(prev_myip) - 1] = '\0';
            }

            /* Target PC: "[ON]  192.168.1.50" or "[OFF]  192.168.1.50" */
            char new_target[32];
            snprintf(new_target, sizeof(new_target), "%s  %s",
                     last_known_state ? "[ON]" : "[OFF]",
                     target_pc_ip);

            if (strcmp(new_target, prev_target_str) != 0) {
                lv_label_set_text(lbl_target, new_target);
                strncpy(prev_target_str, new_target, sizeof(prev_target_str) - 1);
                prev_target_str[sizeof(prev_target_str) - 1] = '\0';
            }
        }

        lv_task_handler();

        k_mutex_unlock(&display_mutex);
    }
}

K_THREAD_DEFINE(display_tid, 8192, display_task_entry, NULL, NULL, NULL, 7, 0, 0);
