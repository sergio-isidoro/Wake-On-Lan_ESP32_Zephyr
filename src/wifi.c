#include "wifi.h"
#include "notify.h"
#include "storage.h"
#include "shared.h"

#define WOL_PORT    9

/* --- PING --- */
static struct net_icmp_ctx ping_ctx;
static struct k_work_delayable ping_work;
static volatile bool target_is_online  = false;
static bool first_ping_done            = false;
static uint16_t ping_seq               = 0;
static uint8_t ping_attempts           = 0;
static bool wifi_connected             = false;

/* Configuration */
static char saved_ssid[32];
static char saved_pass[64];
static char saved_mac_str[18];
static uint8_t target_mac_bin[6];

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct k_work wol_work;
static struct k_work_delayable reconnect_work;

/* --- MAC PARSE --- */
static void parse_mac_address(const char *mac_str, uint8_t *mac_bin)
{
    for (int i = 0; i < 6; i++) {
        char byte_str[3] = { mac_str[i * 3], mac_str[i * 3 + 1], '\0' };
        mac_bin[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }
}

/* --- RECONNECT --- */
static void reconnect_worker(struct k_work *work)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid        = (uint8_t *)saved_ssid,
        .ssid_length = strlen(saved_ssid),
        .psk         = (uint8_t *)saved_pass,
        .psk_length  = strlen(saved_pass),
        .channel     = WIFI_CHANNEL_ANY,
        .security    = WIFI_SECURITY_TYPE_PSK,
        .band        = WIFI_FREQ_BAND_2_4_GHZ,
    };
    printk("[WIFI] Reconnecting to %s...\n", saved_ssid);
    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}

/* --- WAKE-ON-LAN --- */
static void send_wol_worker(struct k_work *work)
{
    if (!SHARED->has_ip) {
        printk("[WOL] No IP — WoL cancelled\n");
        return;
    }

    uint8_t pkt[102];
    struct sockaddr_in dest;

    memset(pkt, 0xFF, 6);
    for (int i = 1; i <= 16; i++) {
        memcpy(&pkt[i * 6], target_mac_bin, 6);
    }

    int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("[WOL] socket error: %d\n", sock);
        return;
    }

    int opt = 1;
    zsock_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(WOL_PORT);
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    if (zsock_sendto(sock, pkt, sizeof(pkt), 0,
                     (struct sockaddr *)&dest, sizeof(dest)) > 0) {
        printk("[WIFI] WoL Magic Packet sent to %s\n", saved_mac_str);
        /* Notify display (core 1) and LED */
        SHARED->wol_sent = true;
        shared_sync();
        notify_event(NOTIFY_WOL_SENT);
    } else {
        printk("[WOL] Send error: %d\n", errno);
    }

    zsock_close(sock);
}

void trigger_wol(void)
{
    k_work_submit(&wol_work);
}

/* --- PING --- */
static int ping_handler(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
                        struct net_icmp_ip_hdr *ip_hdr,
                        struct net_icmp_hdr *icmp_hdr, void *user_data)
{
    target_is_online = true;
    return 0;
}

static void ping_worker_handler(struct k_work *work)
{
    if (ping_attempts > 0) {
        if (target_is_online) {
            if (!first_ping_done || !SHARED->last_known_state) {
                SHARED->last_known_state = true;
                shared_sync();
                first_ping_done = true;
                notify_event(NOTIFY_PING_UPDATE);
            }
            ping_attempts = 0;
            k_work_reschedule(&ping_work, K_MINUTES(1));
            return;
        } else if (ping_attempts >= 3) {
            if (!first_ping_done || SHARED->last_known_state) {
                SHARED->last_known_state = false;
                shared_sync();
                first_ping_done = true;
                notify_event(NOTIFY_PING_UPDATE);
            }
            ping_attempts = 0;
            k_work_reschedule(&ping_work, K_MINUTES(1));
            return;
        }
    }

    target_is_online = false;
    ping_attempts++;

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    zsock_inet_pton(AF_INET, (const char *)SHARED->target_ip, &dest.sin_addr);

    struct net_icmp_ping_params params = {
        .identifier = 0x1234,
        .sequence   = ping_seq++,
    };

    net_icmp_send_echo_request(&ping_ctx, net_if_get_default(),
                               (struct sockaddr *)&dest, &params, NULL);
    k_work_reschedule(&ping_work, K_SECONDS(1));
}

/* --- CALLBACKS --- */
static void handle_wifi_events(struct net_mgmt_event_callback *cb,
                                uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        wifi_connected = true;
        printk("[WIFI] Connected!\n");
        net_dhcpv4_start(iface);
    }

    if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        if (!wifi_connected) return;
        wifi_connected = false;
        printk("[WIFI] Disconnected — reconnecting in 3s...\n");
        net_dhcpv4_stop(iface);
        SHARED->last_known_state = false;
        SHARED->has_ip           = false;
        SHARED->my_ip[0]         = '\0';
        shared_sync();
        k_work_reschedule(&reconnect_work, K_SECONDS(3));
    }
}

static void handle_ipv4_events(struct net_mgmt_event_callback *cb,
                                uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
        if (!ipv4) return;

        for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
            struct net_if_addr *addr = &ipv4->unicast[i].ipv4;
            if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED) {
                char ip_buf[16];
                zsock_inet_ntop(AF_INET,
                    &ipv4->unicast[i].ipv4.address.in_addr,
                    ip_buf, sizeof(ip_buf));
                strncpy((char *)SHARED->my_ip, ip_buf, sizeof(SHARED->my_ip) - 1);
                SHARED->has_ip = true;
                shared_sync();
                k_work_reschedule(&ping_work, K_NO_WAIT);
                break;
            }
        }
    }
}

/* --- WI-FI INIT --- */
void wifi_init_and_connect(const char *ssid, const char *pass,
                           const char *mac, const char *pc_ip)
{
    strncpy(saved_ssid,    ssid, sizeof(saved_ssid) - 1);
    saved_ssid[sizeof(saved_ssid) - 1] = '\0';

    strncpy(saved_pass,    pass, sizeof(saved_pass) - 1);
    saved_pass[sizeof(saved_pass) - 1] = '\0';

    strncpy(saved_mac_str, mac, sizeof(saved_mac_str) - 1);
    saved_mac_str[sizeof(saved_mac_str) - 1] = '\0';

    /* Write target IP into shared struct so core 1 can display it */
    strncpy((char *)SHARED->target_ip, pc_ip, sizeof(SHARED->target_ip) - 1);
    SHARED->target_ip[sizeof(SHARED->target_ip) - 1] = '\0';
    shared_sync();

    parse_mac_address(saved_mac_str, target_mac_bin);

    k_work_init(&wol_work, send_wol_worker);
    k_work_init_delayable(&ping_work, ping_worker_handler);
    k_work_init_delayable(&reconnect_work, reconnect_worker);
    net_icmp_init_ctx(&ping_ctx, AF_INET, NET_ICMPV4_ECHO_REPLY, 0, ping_handler);

    net_mgmt_init_event_callback(&wifi_cb, handle_wifi_events,
                                 NET_EVENT_WIFI_CONNECT_RESULT |
                                 NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    net_mgmt_init_event_callback(&ipv4_cb, handle_ipv4_events,
                                 NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);

    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid        = (uint8_t *)saved_ssid,
        .ssid_length = strlen(saved_ssid),
        .psk         = (uint8_t *)saved_pass,
        .psk_length  = strlen(saved_pass),
        .channel     = WIFI_CHANNEL_ANY,
        .security    = WIFI_SECURITY_TYPE_PSK,
        .band        = WIFI_FREQ_BAND_2_4_GHZ,
    };

    printf("[WIFI] Connecting to %s...\n", saved_ssid);
    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}
