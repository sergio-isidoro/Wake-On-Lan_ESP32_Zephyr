/* SPDX-License-Identifier: Apache-2.0
 *
 * Wake On LAN — ESP32 DevKitC
 * - Watchdog 5 s (MWDT via wdt1)
 *
 * Note: CONFIG_SMP / k_thread_cpu_pin are NOT used because WIFI_ESP32
 * depends on (!SMP). The display thread runs at priority 7, Wi-Fi tasks
 * at higher priority — the Zephyr scheduler handles interleaving.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include "display.h"
#include "storage.h"
#include "wifi.h"
#include "button.h"
#include "notify.h"
#include "portal.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* ── Watchdog ───────────────────────────────────────────────────────────────
 * Timeout: 5 s.  A dedicated low-priority thread feeds every 2.5 s.
 */
#define WDT_TIMEOUT_MS  5000

static const struct device *wdt_dev = DEVICE_DT_GET(DT_NODELABEL(wdt1));
static int wdt_channel_id;

static void wdt_feed_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        wdt_feed(wdt_dev, wdt_channel_id);
        k_msleep(WDT_TIMEOUT_MS / 2);
    }
}
K_THREAD_DEFINE(wdt_tid, 512, wdt_feed_thread, NULL, NULL, NULL, 14, 0, 0);

static void watchdog_init(void)
{
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("WDT device not ready");
        return;
    }

    struct wdt_timeout_cfg cfg = {
        .window   = { .min = 0, .max = WDT_TIMEOUT_MS },
        .callback = NULL,
        .flags    = WDT_FLAG_RESET_SOC,
    };

    wdt_channel_id = wdt_install_timeout(wdt_dev, &cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("WDT install timeout failed: %d", wdt_channel_id);
        return;
    }

    if (wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG) < 0) {
        LOG_ERR("WDT setup failed");
        return;
    }

    LOG_INF("WDT armed — timeout %d ms", WDT_TIMEOUT_MS);
}

/* ── main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    printk("\n=== Wake On LAN — ESP32 ===\n");

    watchdog_init();
    notify_init();
    button_init();
    storage_init();

    char ssid[32]               = {0};
    char pass[64]               = {0};
    char mac[18]                = {0};
    char pc_ip[INET_ADDRSTRLEN] = {0};

    if (storage_read_config(ssid, pass, mac, pc_ip) == 0) {
        LOG_INF("Credentials found — connecting to: %s", ssid);
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
