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
 * Global variables to hold network state and configuration.
 * These are defined in wifi.c and accessed by other modules as needed.
 */
extern char global_ip_str[INET_ADDRSTRLEN];
extern bool last_known_state;
extern char target_pc_ip[INET_ADDRSTRLEN];
 
/**
 * @brief Initializes the Wi-Fi stack and attempts to connect.
 *
 * @param ssid The Wi-Fi network name from Flash.
 * @param pass The Wi-Fi password from Flash.
 * @param mac  The target PC MAC address (string format "AA:BB:CC:DD:EE:FF").
 * @param pc_ip The target PC IP address (string format "192.168.1.100").
 */
void wifi_init_and_connect(const char *ssid, const char *pass, const char *mac, const char *pc_ip);
 
/**
 * @brief Submits a Wake-on-LAN Magic Packet task to the system workqueue.
 *
 * This is called by the button interrupt handler in button.c.
 */
void trigger_wol(void);
 
#endif /* WIFI_H */