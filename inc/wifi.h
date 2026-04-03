#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dhcpv4.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/**
 * @brief Initialise Wi-Fi and connect to the saved network.
 * Also writes target_ip into the shared struct so core 1 can display it.
 */
void wifi_init_and_connect(const char *ssid, const char *pass,
                           const char *mac, const char *pc_ip);

/**
 * @brief Submit a Wake-on-LAN magic packet to the system workqueue.
 * Called by button.c on BOOT button press.
 */
void trigger_wol(void);

#endif /* WIFI_H */
