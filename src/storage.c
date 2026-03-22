#include "storage.h"

static struct nvs_fs fs;

/* Use the 'nvs_storage' label defined in app.overlay to avoid redefinition errors */
#define STORAGE_NODE DT_NODE_BY_FIXED_PARTITION_LABEL(nvs_storage)
#define FLASH_NODE DT_MTD_FROM_FIXED_PARTITION(STORAGE_NODE)

void storage_init(void) {
    int rc;
    struct flash_pages_info info;
    const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);

    if (!device_is_ready(flash_dev)) {
        printk("Error: Flash device not ready\n");
        return;
    }

    fs.flash_device = flash_dev;
    fs.offset = DT_REG_ADDR(STORAGE_NODE);
    
    rc = flash_get_page_info_by_offs(flash_dev, fs.offset, &info);
    if (rc) {
        printk("Error getting page info (%d)\n", rc);
        return;
    }

    fs.sector_size = info.size;
    fs.sector_count = 3; /* Reserve 3 sectors for NVS */

    rc = nvs_mount(&fs);
    if (rc) {
        printk("Error mounting NVS (%d)\n", rc);
    }
}

int storage_write_config(const char *s, const char *p, const char *m, const char *ip) {
    nvs_write(&fs, STORAGE_ID_SSID, s,  strlen(s)  + 1);
    nvs_write(&fs, STORAGE_ID_PASS, p,  strlen(p)  + 1);
    nvs_write(&fs, STORAGE_ID_MAC,  m,  strlen(m)  + 1);
    nvs_write(&fs, STORAGE_ID_IP,   ip, strlen(ip) + 1);
    return 0;
}

int storage_read_config(char *s, char *p, char *m, char *ip) {
    if (nvs_read(&fs, STORAGE_ID_SSID, s, 32) <= 0) return -1;
    nvs_read(&fs, STORAGE_ID_PASS, p,  64);
    nvs_read(&fs, STORAGE_ID_MAC,  m,  18);
    nvs_read(&fs, STORAGE_ID_IP,   ip, INET_ADDRSTRLEN);
    return 0;
}

void storage_clear_all(void) {
    nvs_delete(&fs, STORAGE_ID_SSID);
    nvs_delete(&fs, STORAGE_ID_PASS);
    nvs_delete(&fs, STORAGE_ID_MAC);
    nvs_delete(&fs, STORAGE_ID_IP);
}