#ifndef DISPLAY_H
#define DISPLAY_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/sys/reboot.h>

/** @brief Semaphore to start the display task. */
extern struct k_sem sem_display_start;
extern struct k_sem sem_ui_refresh;

/** @brief Give this semaphore to show "WoL sent!" for 1.5 s. */
extern struct k_sem sem_wol_sent;

/** @brief Display state flags. */
extern bool display_ap_mode;
extern bool display_wifi_ready;
extern bool display_station_ready;
extern bool has_ip;

void display_task_entry(void *p1, void *p2, void *p3);

/** @brief Display thread ID — used by main() to pin it to core 1. */
extern const k_tid_t display_tid;

#endif /* DISPLAY_H */
