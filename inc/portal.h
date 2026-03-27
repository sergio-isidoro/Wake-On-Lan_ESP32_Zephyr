#ifndef PORTAL_H
#define PORTAL_H

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/reboot.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Starts the Captive Portal.
 * * This function performs the following steps:
 * 1. Enables WiFi Access Point mode (SSID: WOL_ESP).
 * 2. Sets a static IP (192.168.4.1).
 * 3. Starts the DNS redirector task to intercept all requests.
 * 4. Starts the HTTP server to serve the configuration form.
 */
void start_portal(void);

/**
 * @brief Stops the Captive Portal and disables AP mode.
 * * This function performs the following steps:
 * 1. Disables WiFi Access Point mode.
 * 2. Resets the network interface to default state.
 */
void stop_portal(void);

#endif /* PORTAL_H */