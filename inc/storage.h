#ifndef STORAGE_H
#define STORAGE_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/net/net_ip.h>
#include <string.h>
#include <zephyr/types.h>

/* Unique IDs for each entry in the NVS file system.
 * These IDs are used as "keys" to access data in Flash.
 */
#define STORAGE_ID_SSID     1
#define STORAGE_ID_PASS     2
#define STORAGE_ID_MAC      3
#define STORAGE_ID_IP       4
 
/**
 * @brief Initializes the NVS storage subsystem.
 * Locates the partition defined in the devicetree and mounts the file system.
 */
void storage_init(void);
 
/**
 * @brief Saves the Wi-Fi settings and target PC MAC address to Flash.
 *
 * @param s Wi-Fi network name (SSID).
 * @param p Network password.
 * @param m Target PC MAC address (string format "AA:BB:CC:DD:EE:FF").
 * @return 0 on success, or negative error code.
 */
int storage_write_config(const char *s, const char *p, const char *m, const char *ip);
 
/**
 * @brief Reads the saved settings from Flash.
 *
 * @param s Buffer to store the SSID (min. 32 bytes).
 * @param p Buffer to store the Password (min. 64 bytes).
 * @param m Buffer to store the MAC (min. 18 bytes).
 * @return 0 if data was read successfully, -1 if Flash is empty.
 */
int storage_read_config(char *s, char *p, char *m, char *ip);
 
/**
 * @brief Erases all settings from Flash (Factory Reset).
 * Removes the SSID, Password and MAC, forcing the device into Portal mode.
 */
void storage_clear_all(void);
 
#endif /* STORAGE_H */