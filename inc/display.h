#ifndef DISPLAY_H
#define DISPLAY_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/sys/reboot.h>
 
/** @brief Semaphore for starting the display task. */
extern struct k_sem sem_display_start;
extern struct k_sem sem_ui_refresh;
 
/** @brief Flag indicating if the display is in AP mode. */
extern bool display_ap_mode;
extern bool display_wifi_ready;
extern bool display_station_ready;
extern bool has_ip;

/**
 * @brief Entry point for the display management thread.
 *
 * This function initializes the display, shows the appropriate UI based on the mode (AP or Station),
 * and updates the screen with Wi-Fi status and IP address information.
 *
 * @param p1 Unused parameter for thread entry.
 * @param p2 Unused parameter for thread entry.
 * @param p3 Unused parameter for thread entry.
 */
void display_task_entry(void *p1, void *p2, void *p3);
 
#endif /* DISPLAY_H */
 