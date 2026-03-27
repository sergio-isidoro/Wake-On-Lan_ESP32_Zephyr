#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <stdio.h>

#include "storage.h"
#include "notify.h"
#include "button.h"
#include "wifi.h"
#include "portal.h"
#include "display.h"

#define WDT_TIMEOUT_MS              1500        /* Watchdog timeout in milliseconds */

int main(void) {
    /* Short delay to allow system stabilization after boot */
    k_msleep(100);
    
    /* Watchdog Timer Setup */
    const struct device *wdt_dev = DEVICE_DT_GET(DT_NODELABEL(wdt1));
    int wdt_channel = -1;                                                       // Will hold the channel ID if WDT is successfully initialized

    /* Buffers for configuration data */
    char ssid[32], pass[64], mac[18], pc_ip[INET_ADDRSTRLEN];

    /* Hardware and Subsystem Initialization */
    storage_init();
    notify_init();
    button_init();

    /* Configure WDT with a timeout and set it to reset the SoC on expiry */
    if (device_is_ready(wdt_dev)) {
        struct wdt_timeout_cfg wdt_config = {.window.max = WDT_TIMEOUT_MS, .flags = WDT_FLAG_RESET_SOC};
        wdt_channel = wdt_install_timeout(wdt_dev, &wdt_config);
        wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
    }

    /* Flow Decision */
    int rc = storage_read_config(ssid, pass, mac, pc_ip);

    if (rc == 0 && strlen(ssid) > 0 && strlen(mac) == 17) {
        printf("[SYSTEM] Config loaded. Starting Station mode...\n");
        wifi_init_and_connect(ssid, pass, mac, pc_ip);                  // This function will block until WiFi is connected and IP is obtained, then return to main loop
    } else {
        if (rc == 0) {
            printf("[SYSTEM] Invalid config in Flash. Clearing...\n");
            storage_clear_all();                                        // Clear invalid config to allow fresh start on next boot
        }
        printf("[SYSTEM] Starting Portal...\n");
        start_portal();                                                 // This function will block and run the WiFi AP and configuration portal until the user completes setup, then it will reboot the system to apply the new config
    }

    printf("\n--- Wake-on-LAN ESP32-C3 SuperMini ---\n");

    /* Main Loop */
    while (1) {
        
        /* Check if no IP address */
        if (!has_ip) {
            /* Check for factory reset button press */
            if (button_is_pressed()) {
                printf("[SYSTEM] FACTORY RESET ACTIVATED!\n");
                storage_clear_all();                                // Clear all stored configurations
                notify_event(NOTIFY_WOL_SENT);                      // Notify user of reset (2 blinks)
                k_msleep(1000);                                     // Wait a moment to ensure notification is seen
                sys_reboot(SYS_REBOOT_COLD);                        // Reboot the system to start fresh
            }
        }

        /* Feed the watchdog to prevent reset */
        if (wdt_channel >= 0){
            wdt_feed(wdt_dev, wdt_channel);
        }

        k_msleep(500);                                              // Sleep for a while before checking again
    }

    return 0;
}