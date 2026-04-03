/*
 * button.c — BOOT button handler
 *
 * Short press  (< 1 s)              → send WoL packet
 * Long press   (≥ 1 s, !has_ip)    → factory reset
 *
 * A dedicated thread (not sysworkq) handles the blocking press_worker_fn.
 * The sysworkq stack is too small for k_msleep + storage + reboot calls.
 */

#include "button.h"
#include "wifi.h"
#include "storage.h"
#include "shared.h"
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(button, CONFIG_LOG_DEFAULT_LEVEL);

/* ── GPIO ────────────────────────────────────────────────────────────────── */
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback btn_cb_data;

/* ── Thresholds ──────────────────────────────────────────────────────────── */
#define LONG_PRESS_MS  1000U
#define POLL_MS          20U

/* ── Dedicated button thread ─────────────────────────────────────────────── */
#define BTN_STACK_SIZE  2048
#define BTN_PRIORITY    5

static K_THREAD_STACK_DEFINE(btn_stack, BTN_STACK_SIZE);
static struct k_thread btn_thread_data;

/* Semaphore: ISR signals thread on each press */
static K_SEM_DEFINE(btn_sem, 0, 1);
static int64_t press_start_ms;

static void button_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (1) {
        /* Wait for ISR to signal a press */
        k_sem_take(&btn_sem, K_FOREVER);

        /* Poll until released */
        while (gpio_pin_get_dt(&btn) == 1) {
            k_msleep(POLL_MS);
        }

        int64_t held_ms = k_uptime_get() - press_start_ms;

        if (held_ms >= LONG_PRESS_MS) {
            if (!SHARED->has_ip) {
                LOG_WRN("Factory reset (held %lld ms)", held_ms);
                SHARED->factory_reset = true;
                shared_sync();
                k_msleep(2000);          /* let display show the bar */
                storage_clear_all();
                LOG_WRN("NVS cleared — rebooting");
                sys_reboot(SYS_REBOOT_COLD);
            } else {
                LOG_INF("Long press ignored — already connected");
            }
        } else {
            LOG_INF("Short press → WoL");
            trigger_wol();
        }
    }
}

/* ── ISR — signals thread on falling edge ────────────────────────────────── */
static void button_isr(const struct device *dev,
                       struct gpio_callback *cb,
                       uint32_t pins)
{
    press_start_ms = k_uptime_get();
    k_sem_give(&btn_sem);
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void button_init(void)
{
    if (!gpio_is_ready_dt(&btn)) {
        LOG_ERR("BOOT button GPIO not ready");
        return;
    }

    gpio_pin_configure_dt(&btn, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_cb_data, button_isr, BIT(btn.pin));
    gpio_add_callback(btn.port, &btn_cb_data);

    k_thread_create(&btn_thread_data, btn_stack,
                    K_THREAD_STACK_SIZEOF(btn_stack),
                    button_thread, NULL, NULL, NULL,
                    BTN_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&btn_thread_data, "button");

    LOG_INF("Button init OK (long-press=%d ms → factory reset)", LONG_PRESS_MS);
}

bool button_is_pressed(void)
{
    return (gpio_pin_get_dt(&btn) == 1);
}
