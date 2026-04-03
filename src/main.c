/* SPDX-License-Identifier: Apache-2.0
 *
 * Wake On LAN — ESP32 DevKitC
 *
 * main() sequence:
 *   1. watchdog_init()
 *   2. notify_init() / button_init() / storage_init()
 *   3. Read NVS, set SHARED->ap_mode
 *   4. wifi_init_and_connect() or start_portal() — runs forever
 *
 * Display thread is started automatically via K_THREAD_DEFINE in display.c.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <string.h>

#include "shared.h"
#include "display.h"
#include "storage.h"
#include "wifi.h"
#include "button.h"
#include "notify.h"
#include "portal.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* ── Global shared state ─────────────────────────────────────────────────── */

shared_t g_shared;

/* ── Watchdog ────────────────────────────────────────────────────────────── */

#define WDT_TIMEOUT_MS 8000

static const struct device *wdt_dev = DEVICE_DT_GET(DT_NODELABEL(wdt1));
static int wdt_channel_id;

static void wdt_feed_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    while (1) {
        wdt_feed(wdt_dev, wdt_channel_id);
        k_msleep(WDT_TIMEOUT_MS / 2);
    }
}
K_THREAD_DEFINE(wdt_tid, 512, wdt_feed_thread, NULL, NULL, NULL, 14, 0, 0);

static void watchdog_init(void)
{
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("WDT not ready");
        return;
    }
    struct wdt_timeout_cfg cfg = { .window = { .min = 0, .max = WDT_TIMEOUT_MS }, .callback = NULL, .flags = WDT_FLAG_RESET_SOC };
    wdt_channel_id = wdt_install_timeout(wdt_dev, &cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("WDT install: %d", wdt_channel_id);
        return;
    }
    if (wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG) < 0) {
        LOG_ERR("WDT setup failed");
        return;
    }
    LOG_INF("WDT armed %d ms", WDT_TIMEOUT_MS);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printk("\n=== Wake On LAN — ESP32 ===\n");

    memset(&g_shared, 0, sizeof(g_shared));

    watchdog_init();
    notify_init();
    button_init();
    storage_init();

    char ssid[32] = {0};
    char pass[64] = {0};
    char mac[18]  = {0};
    char pc_ip[INET_ADDRSTRLEN] = {0};

    bool has_creds = (storage_read_config(ssid, pass, mac, pc_ip) == 0);
    SHARED->ap_mode = !has_creds;

    if (has_creds) {
        LOG_INF("Connecting to: %s", ssid);
        wifi_init_and_connect(ssid, pass, mac, pc_ip);
    } else {
        LOG_INF("No credentials — starting captive portal");
        start_portal();
    }

    while (1) {
        k_sleep(K_FOREVER);
    }
    return 0;
}
