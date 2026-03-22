#ifndef PORTAL_H
#define PORTAL_H

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>      // zsock_*, SOL_SOCKET, SO_REUSEADDR, AF_INET, etc.
#include <zephyr/net/wifi_mgmt.h>   // wifi_connect_req_params, NET_REQUEST_WIFI_AP_ENABLE, etc.
#include <zephyr/net/net_if.h>      // net_if_get_default, net_if_ipv4_*, NET_ADDR_MANUAL
#include <zephyr/net/net_mgmt.h>    // net_mgmt()
#include <zephyr/net/net_ip.h>      // zsock_inet_pton, in_addr
#include <zephyr/sys/reboot.h>      // sys_reboot, SYS_REBOOT_COLD
#include <stdlib.h>                 // strtol, atoi
#include <string.h>
#include <ctype.h>                  // isxdigit

/**
 * @brief Starts the Captive Portal.
 * * This function performs the following steps:
 * 1. Enables WiFi Access Point mode (SSID: WOL_Config_ESP32).
 * 2. Sets a static IP (192.168.1.1).
 * 3. Starts the DNS redirector task to intercept all requests.
 * 4. Starts the HTTP server to serve the configuration form.
 */
void start_portal(void);

/**
 * @brief Stops all portal-related services.
 * * Useful if you want to transition back to Station mode without 
 * a full system reboot.
 */
void stop_portal(void);

#endif /* PORTAL_H */