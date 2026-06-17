/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "lwip/apps/http_client.h"
#include <string.h>
#include "pan_test/http.h"


static struct
{
    char ip_str[20];
    uint16_t port;
    char uri[80];
} mgr =
{
    .ip_str = {0},
    .port = 80,
    .uri = {0},

};


void app_bt_pan_set_http(char *ip_str, uint32_t ip_str_len, uint16_t port, char *uri,
                         uint32_t uri_len)
{
    strncpy(mgr.ip_str, ip_str, sizeof(mgr.ip_str) - 1);
    mgr.port = port;
    strncpy(mgr.uri, uri, sizeof(mgr.uri) - 1);
}


static void httpc_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len,
                         u32_t srv_res, err_t err)
{
    LWIP_PLATFORM_DIAG(("httpc_result_fn: httpc_result_t %d rx_content_len %d srv_res %d err %d",
                        httpc_result, rx_content_len, srv_res, err));
}

static err_t httpc_headers_done(httpc_state_t *connection, void *arg, struct pbuf *hdr,
                                u16_t hdr_len, u32_t content_len)
{
    LWIP_PLATFORM_DIAG(("httpc_headers_done: rx_http_version %d rx_content_len %d  rx_status %d",
                        connection->rx_http_version, connection->rx_content_len, connection->rx_status));
    LWIP_PLATFORM_DIAG(("httpc_headers_done: %s", (char *)connection->rx_hdrs->payload));
    return ERR_OK;
}

static err_t own_recv(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    uint8_t *data = (uint8_t *)p->payload;

    LWIP_PLATFORM_DIAG(("own_recv: %02X %02X %02X %02X",
                        data[0], data[1], data[2], data[3]));
    tcp_recved(conn, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}


bool app_bt_pan_demo_http(void)
{
    static ip_addr_t ip_addr;
    static httpc_connection_t settings = {.use_proxy = 0, .result_fn = httpc_result, .headers_done_fn = httpc_headers_done};
//    static char *uri = "/abc.zip";
//    static char* server_name = "t.weather.sojson.com";
//    const char *ip_str = "192.168.44.17";
//    LWIP_PLATFORM_DIAG(("app_bt_pan_demo_ping: ip %s", ip_str));
    ipaddr_aton(mgr.ip_str, &ip_addr);

    httpc_get_file(&ip_addr, mgr.port, mgr.uri, &settings, own_recv, NULL, NULL);


//    httpc_get_file_dns(server_name, 80, uri, &settings, own_recv, NULL, NULL);

//    extern struct netif loop_netif;
//    LWIP_PLATFORM_DIAG(("app_bt_pan_demo_ping: loop addr %s", ip6addr_ntoa(ip_2_ip6(loop_netif.ip6_addr))));
//    const char *ip_str = "110.242.68.66";
//    LWIP_PLATFORM_DIAG(("app_bt_pan_demo_ping: loop addr %s", ip6addr_ntoa(ip_2_ip6(loop_netif.ip6_addr))));
//    ipaddr_aton(ip_str, &ip_addr);
//    ping_init(&ip_addr);
//    ping_send_now();

    return true;
}

