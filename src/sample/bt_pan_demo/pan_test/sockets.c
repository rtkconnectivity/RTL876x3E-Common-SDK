/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#include "pan_test/sockets.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "os_sched.h"
#include <stdio.h>
static struct
{
    char buf[500];
    char ip_str[20];
    uint16_t port;

    enum APP_BT_PAN_SOCKETS_MODE mode;

    struct
    {
        uint32_t total_rate;
        uint32_t current_rate;
        uint32_t report_interval;
    } server;
} mgr =
{
    .mode = APP_BT_PAN_SOCKETS_INVALID,
    .buf = {0},
    .ip_str = {0},
    .port = 0xffff,
    .server =
    {
        .total_rate = 0,
        .current_rate = 0,
        .report_interval = 0,
    }
};

void app_bt_pan_sockets_set_ip_str(char *ip_str, uint32_t len)
{
    strncpy(mgr.ip_str, ip_str, sizeof(mgr.ip_str) - 1);
}

char *app_bt_pan_sockets_get_ip_str(void)
{
    return mgr.ip_str;
}

void app_bt_pan_sockets_set_port(uint16_t port)
{
    mgr.port = port;
}


uint16_t app_bt_pan_sockets_get_port(void)
{
    return mgr.port;
}

void app_bt_pan_sockets_set_mode(char *mode_str)
{
    if (strncmp("client", mode_str, 6) == 0)
    {
        mgr.mode = APP_BT_PAN_SOCKETS_CLIENT;
    }
    else if (strncmp("server", mode_str, 6) == 0)
    {
        mgr.mode = APP_BT_PAN_SOCKETS_SERVER;
    }
}


enum APP_BT_PAN_SOCKETS_MODE app_bt_pan_sockets_get_mode(void)
{
    return mgr.mode;
}


static void client(void *param)
{
    int sock = -1;
    struct sockaddr_in client_addr;
    static uint32_t seq = 0;

    const char str[] = "This is a TCP Client test %u\n";
    LWIP_PLATFORM_DIAG(("client: start\n"));
    static ip_addr_t ip_addr;
    ipaddr_aton(app_bt_pan_sockets_get_ip_str(), &ip_addr);
//    IP4_ADDR(&ipaddr, 192, 168, 44, 17);
    while (1)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            LWIP_PLATFORM_DIAG(("client: Socket error\n"));
            os_delay(10);
        }

        // ????????????
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags < 0)
        {
            LWIP_PLATFORM_DIAG(("client: fcntl(F_GETFL) failed"));
            close(sock);
        }
        else
        {
            LWIP_PLATFORM_DIAG(("client: flags %08x", flags));
        }

        // ??????? (O_NONBLOCK)
        flags &= ~O_NONBLOCK;

        // ????????
        if (fcntl(sock, F_SETFL, flags) < 0)
        {
            LWIP_PLATFORM_DIAG(("client: fcntl(F_SETFL) failed"));
            close(sock);
        }


        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(app_bt_pan_sockets_get_port());
        client_addr.sin_addr.s_addr = ip_addr.addr;
        memset(&(client_addr.sin_zero), 0, sizeof(client_addr.sin_zero));

        if (connect(sock,
                    (struct sockaddr *)&client_addr,
                    sizeof(struct sockaddr)) == -1)
        {
            LWIP_PLATFORM_DIAG(("client: Connect failed!\n"));
            closesocket(sock);
            os_delay(10);
            continue;
        }

        LWIP_PLATFORM_DIAG(("client: Connect to server successful!\n"));

        while (1)
        {
            snprintf(mgr.buf, sizeof(mgr.buf), str, seq);
            seq++;
            write(sock, mgr.buf, 1500);
            snprintf(mgr.buf, sizeof(mgr.buf), str, seq);
            seq++;
            write(sock, mgr.buf, 1500);
            snprintf(mgr.buf, sizeof(mgr.buf), str, seq);
            seq++;
            write(sock, mgr.buf, 1500);
            os_delay(20);
        }
//        closesocket(sock);
    }
}


void server(void *param)
{
    int server_fd = 0xf, client_fd = 0xf;
    struct sockaddr_in address = {};
    uint32_t start_time, end_time, report_start_time;
    int32_t total_size = 0, report_size = 0;
    int addrlen = sizeof(address);
    mgr.server.report_interval = 1; //(s)

    LWIP_PLATFORM_DIAG(("tcp_server start"));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        LWIP_PLATFORM_DIAG(("server: Socket error\n"));
        os_delay(10);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(app_bt_pan_sockets_get_port());

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LWIP_PLATFORM_DIAG(("bind failed"));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // ????
    if (listen(server_fd, 3) < 0)
    {
        LWIP_PLATFORM_DIAG(("listen"));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            LWIP_PLATFORM_DIAG(("accept"));
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        start_time = sys_now();
        end_time = start_time;
        report_start_time = start_time;
        total_size = 0;
        report_size = 0;

        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags < 0)
        {
            LWIP_PLATFORM_DIAG(("server: fcntl F_GETFL failed"));
            close(client_fd);
            return;
        }

        flags &= ~O_NONBLOCK;  // ???????
        if (fcntl(client_fd, F_SETFL, flags) < 0)
        {
            LWIP_PLATFORM_DIAG(("fcntl F_SETFL failed"));
            close(client_fd);
            return;
        }

        while (1)
        {
            int len = read(client_fd, mgr.buf, sizeof(mgr.buf));
            LWIP_PLATFORM_DIAG(("server: read len %d\n", len));

            total_size += len;
            report_size += len;
            end_time = sys_now();

            if ((end_time - report_start_time) >= (1000 * mgr.server.report_interval))
            {
                mgr.server.current_rate = (uint32_t)(report_size * 8) / (end_time - report_start_time);
                LWIP_PLATFORM_DIAG(("TCP: Receive %d KBytes in %d ms, %d Kbits/sec", (uint32_t)(report_size / 1024),
                                    (uint32_t)(end_time - report_start_time),
                                    mgr.server.current_rate));
                report_start_time = end_time;
                report_size = 0;
            }
        }
    }
}


bool app_bt_pan_sockets(void)
{
    if (mgr.mode == APP_BT_PAN_SOCKETS_CLIENT)
    {
        sys_thread_new("client", client, NULL, 2048, 0);
    }
    else if (mgr.mode == APP_BT_PAN_SOCKETS_SERVER)
    {
        sys_thread_new("server", server, NULL, 2048, 0);
    }
    else
    {
        LWIP_PLATFORM_DIAG(("app_bt_pan_sockets: error mode"));
    }

    return true;
}

