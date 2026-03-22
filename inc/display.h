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
 
/* Semaphore to start the display thread (given by wifi.c when IP is acquired) */
extern struct k_sem sem_display_start;
 
/* Semaphore to force an immediate display refresh */
extern struct k_sem sem_ui_refresh;
 
extern bool display_station_ready;

extern bool has_ip;
 
/* State flags */
extern bool display_ap_mode;
extern bool display_wifi_ready;
 
#endif /* DISPLAY_H */
 