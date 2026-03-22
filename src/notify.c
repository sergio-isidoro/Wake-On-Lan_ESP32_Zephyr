#include "notify.h"

extern bool display_wifi_ready;
extern bool display_station_ready;

/* Get LED definition from Devicetree (blue_led must be in your overlay) */
static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);

/* External semaphore defined in display.c */
extern struct k_sem sem_ui_refresh;

void notify_init(void) {
    if (!device_is_ready(blue_led.port)) {
        return;
    }
    /* LED is Active Low on SuperMini, ACTIVE_LOW flag in overlay handles it */
    gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
}

void notify_event(notify_type_t type) {
    int blinks = (type == NOTIFY_WOL_SENT) ? 2 : 1;

    /* Only wake up display in station mode */
    if (display_station_ready) {
        k_sem_give(&sem_ui_refresh);
    }

    for (int i = 0; i < blinks; i++) {
        gpio_pin_set_dt(&blue_led, 1);
        k_msleep(500);
        gpio_pin_set_dt(&blue_led, 0);
        if (blinks > 1 && i < (blinks - 1)) {
            k_msleep(200);
        }
    }
}