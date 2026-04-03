#include "portal.h"
#include "portal_html.h"
#include "storage.h"
#include "notify.h"
#include "shared.h"

static bool portal_active = false;

/* --- HELPERS --- */
static void send_all(int sock, const char *data, int len) {
    while (len > 0) {
        int sent = zsock_send(sock, data, len, 0);
        if (sent <= 0) break;
        data += sent;
        len  -= sent;
    }
}

static void send_form(int client, bool with_err) {
    const char *hdr =
        "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
        "Connection: close\r\n\r\n";
    send_all(client, hdr,     strlen(hdr));
    send_all(client, html_p1, strlen(html_p1));
    send_all(client, html_p2, strlen(html_p2));
    send_all(client, html_p3, strlen(html_p3));
    send_all(client, html_p4, strlen(html_p4));
    if (with_err) {
        send_all(client, html_err, strlen(html_err));
    }
    send_all(client, html_tail, strlen(html_tail));
}

static void extract_field(const char *body, const char *key, char *out, size_t out_len) {
    const char *p = strstr(body, key);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(key) + 1;
    size_t i = 0;
    while (*p && *p != '&' && i < out_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

static void url_decode(char *dst, const char *src, size_t dst_len) {
    size_t i = 0;
    while (*src && i < dst_len - 1) {
        if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], '\0' };
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

static bool is_valid_mac(const char *mac) {
    if (strlen(mac) != 17) return false;
    for (int i = 0; i < 17; i++) {
        if (i % 3 == 2) {
            if (mac[i] != ':') return false;
        } else {
            if (!isxdigit((unsigned char)mac[i])) return false;
        }
    }
    return true;
}

/* --- HTTP SERVER --- */
static void http_server_task(void *p1, void *p2, void *p3) {
    while (!portal_active) {
        k_msleep(100);
    }

    int server = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) {
        printk("[HTTP] Error creating socket\n");
        return;
    }

    int opt = 1;
    zsock_setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(80),
        .sin_addr.s_addr = INADDR_ANY,
    };
    zsock_bind(server, (struct sockaddr *)&addr, sizeof(addr));
    zsock_listen(server, 2);

    static char buf[1024];

    while (1) {
        int client = zsock_accept(server, NULL, NULL);
        if (client < 0) continue;

        int total = 0;
        int content_length = 0;
        bool headers_done = false;
        char *body_start = NULL;

        memset(buf, 0, sizeof(buf));

        while (total < (int)sizeof(buf) - 1) {
            int n = zsock_recv(client, buf + total, sizeof(buf) - 1 - total, 0);
            if (n <= 0) break;
            total += n;
            buf[total] = '\0';

            if (!headers_done) {
                body_start = strstr(buf, "\r\n\r\n");
                if (body_start) {
                    headers_done = true;
                    body_start += 4;
                    char *cl = strstr(buf, "Content-Length:");
                    if (!cl) cl = strstr(buf, "content-length:");
                    if (cl) content_length = atoi(cl + 15);
                }
            }

            if (headers_done && content_length > 0) {
                int body_received = total - (int)(body_start - buf);
                if (body_received >= content_length) break;
            } else if (headers_done && content_length == 0) {
                break;
            }
        }

        if (!headers_done || !body_start) {
            zsock_close(client);
            continue;
        }

        printk("[HTTP] body: %s\n", body_start);

        if (strncmp(buf, "POST /save", 10) == 0) {
            char raw_ssid[32]={0}, raw_pass[64]={0}, raw_mac[64]={0}, raw_ip[32]={0};
            extract_field(body_start, "s", raw_ssid, sizeof(raw_ssid));
            extract_field(body_start, "p", raw_pass, sizeof(raw_pass));
            extract_field(body_start, "i", raw_ip,   sizeof(raw_ip));
            extract_field(body_start, "m", raw_mac,  sizeof(raw_mac));

            char ssid[32]={0}, pass[64]={0}, mac[18]={0}, pc_ip[INET_ADDRSTRLEN]={0};
            url_decode(ssid,  raw_ssid, sizeof(ssid));
            url_decode(pass,  raw_pass, sizeof(pass));
            url_decode(pc_ip, raw_ip,   sizeof(pc_ip));
            url_decode(mac,   raw_mac,  sizeof(mac));

            printk("[PORTAL] SSID='%s' IP='%s' MAC='%s'\n", ssid, pc_ip, mac);

            /* Validate IP */
            struct in_addr tmp;
            bool ip_ok = (strlen(pc_ip) > 0 && zsock_inet_pton(AF_INET, pc_ip, &tmp) == 1);

            if (strlen(ssid) == 0 || !is_valid_mac(mac) || !ip_ok) {
                send_form(client, true);
                zsock_close(client);
                continue;
            }

            storage_write_config(ssid, pass, mac, pc_ip);
            send_all(client, html_ok, strlen(html_ok));
            k_msleep(200);
            zsock_close(client);

            notify_event(NOTIFY_WOL_SENT);
            k_msleep(1000);
            sys_reboot(SYS_REBOOT_COLD);

        } else {
            send_form(client, false);
        }

        zsock_close(client);
    }
}
K_THREAD_DEFINE(http_tid, 4096, http_server_task, NULL, NULL, NULL, 7, 0, 0);

/* --- DHCP SERVER --- */
static void dhcp_server_task(void *p1, void *p2, void *p3) {
    while (!portal_active) {
        k_msleep(100);
    }

    int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("[DHCP] Error creating socket\n");
        return;
    }

    int opt = 1;
    zsock_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(67),
        .sin_addr.s_addr = INADDR_ANY,
    };
    zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    uint8_t buf[300];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int len = zsock_recvfrom(sock, buf, sizeof(buf), 0,
                                 (struct sockaddr *)&client_addr, &addr_len);
        if (len < 240 || buf[0] != 1) continue;

        uint8_t reply[300];
        memset(reply, 0, sizeof(reply));

        reply[0] = 2;
        reply[1] = buf[1];
        reply[2] = buf[2];
        reply[3] = buf[3];
        memcpy(&reply[4],  &buf[4], 4);
        reply[10] = buf[10];
        reply[11] = buf[11];

        reply[16] = 192; reply[17] = 168; reply[18] = 4; reply[19] = 2;
        reply[20] = 192; reply[21] = 168; reply[22] = 4; reply[23] = 1;

        memcpy(&reply[28], &buf[28], 6);

        reply[236] = 99; reply[237] = 130; reply[238] = 83; reply[239] = 99;

        int i = 240;
        uint8_t msg_type = (buf[242] == 1) ? 2 : 5;

        reply[i++] = 53; reply[i++] = 1; reply[i++] = msg_type;
        reply[i++] = 54; reply[i++] = 4;
        reply[i++] = 192; reply[i++] = 168; reply[i++] = 4; reply[i++] = 1;
        reply[i++] = 51; reply[i++] = 4;
        reply[i++] = 0; reply[i++] = 0; reply[i++] = 0x0e; reply[i++] = 0x10;
        reply[i++] = 1; reply[i++] = 4;
        reply[i++] = 255; reply[i++] = 255; reply[i++] = 255; reply[i++] = 0;
        reply[i++] = 3; reply[i++] = 4;
        reply[i++] = 192; reply[i++] = 168; reply[i++] = 4; reply[i++] = 1;
        reply[i++] = 6; reply[i++] = 4;
        reply[i++] = 192; reply[i++] = 168; reply[i++] = 4; reply[i++] = 1;
        reply[i++] = 255;

        struct sockaddr_in dst = {
            .sin_family      = AF_INET,
            .sin_port        = htons(68),
            .sin_addr.s_addr = INADDR_BROADCAST,
        };
        zsock_sendto(sock, reply, i, 0, (struct sockaddr *)&dst, sizeof(dst));
        printk("[DHCP] Offered 192.168.4.2\n");
    }
}
K_THREAD_DEFINE(dhcp_tid, 2048, dhcp_server_task, NULL, NULL, NULL, 7, 0, 0);

/* --- DNS CAPTIVE PORTAL --- */
static void dns_server_task(void *p1, void *p2, void *p3) {
    while (!portal_active) {
        k_msleep(100);
    }

    int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("[DNS] Error creating socket\n");
        return;
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(53),
        .sin_addr.s_addr = INADDR_ANY,
    };
    zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    uint8_t buffer[512];
    while (1) {
        struct sockaddr client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int len = zsock_recvfrom(sock, buffer, sizeof(buffer), 0,
                                 &client_addr, &addr_len);
        if (len > 0) {
            buffer[2] |= 0x80;
            zsock_sendto(sock, buffer, len, 0, &client_addr, addr_len);
        }
    }
}
K_THREAD_DEFINE(dns_tid, 2048, dns_server_task, NULL, NULL, NULL, 8, 0, 0);

/* --- START --- */
void start_portal(void) {

    struct net_if *iface = net_if_get_default();

    struct in_addr gw, mask, my_addr;
    zsock_inet_pton(AF_INET, "192.168.4.1", &my_addr);
    zsock_inet_pton(AF_INET, "192.168.4.1", &gw);
    zsock_inet_pton(AF_INET, "255.255.255.0", &mask);
    net_if_ipv4_set_gw(iface, &gw);
    net_if_ipv4_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
    net_if_ipv4_set_netmask_by_addr(iface, &my_addr, &mask);

    struct wifi_connect_req_params params = {
        .ssid        = "WOL_ESP",
        .ssid_length = 7,
        .security    = WIFI_SECURITY_TYPE_NONE,
        .channel     = 1,
        .band        = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp         = WIFI_MFP_DISABLE,
    };

    printk("[PORTAL] AP Mode Starting...\n");
    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));
    if (ret) {
        printk("[PORTAL] AP enable failed: %d\n", ret);
        return;
    }
    printk("[PORTAL] AP up: 192.168.4.1\n");

    portal_active = true;

}