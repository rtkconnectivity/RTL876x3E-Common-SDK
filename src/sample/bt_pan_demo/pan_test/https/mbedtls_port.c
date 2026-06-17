/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

/*============================================================================*
 *                             Includes
 *============================================================================*/
#include "common.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"


#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include <stdio.h>
#include <string.h>

#include "trace.h"


void mbedtls_tls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    char *file_sep;
    file_sep = strrchr(file, '/');
    if (file_sep)
    {
        file = file_sep + 1;
    }
    mbedtls_printf(" %s: %d %s", TRACE_STRING(file), line, TRACE_STRING(str));
//    mbedtls_platform_printf("[mbedtls] %s:%d %s", file, line, str);
}


//int mbedtls_hardware_poll( void *data, unsigned char *output, size_t len, size_t *olen )
//{
//    *olen = len;
//  uint32_t random_seed_value = 0;
//  uint8_t random_seed_len = sizeof(random_seed_value);
//  uint8_t *rand_ptr = (uint8_t *)output;
//  while (len > 0)
//  {
//      random_seed_value = platform_random(0xFFFFFFFF);
//      uint32_t real_len = (random_seed_len < len ) ? random_seed_len : len;
//      memcpy(rand_ptr, &random_seed_value, real_len);
//      len -= real_len;
//      rand_ptr += real_len;
//  }
//    return 0;
//}

/*
 * Return 0 if the file descriptor is valid, an error otherwise.
 * If for_select != 0, check whether the file descriptor is within the range
 * allowed for fd_set used for the FD_xxx macros and the select() function.
 */
static int check_fd(int fd, int for_select)
{
    if (fd < 0)
    {
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)
    (void) for_select;
#else
    /* A limitation of select() is that it only works with file descriptors
     * that are strictly less than FD_SETSIZE. This is a limitation of the
     * fd_set type. Error out early, because attempting to call FD_SET on a
     * large file descriptor is a buffer overflow on typical platforms. */
    if (for_select && fd >= FD_SETSIZE)
    {
        return MBEDTLS_ERR_NET_POLL_FAILED;
    }
#endif

    return 0;
}

static int net_would_block(const mbedtls_net_context *ctx)
{
    /*
     * Never return 'WOULD BLOCK' on a non-blocking socket
     */
    int val = 0;
    (void)(val);

    if ((fcntl(ctx->MBEDTLS_PRIVATE(fd), F_GETFL, val) & O_NONBLOCK) != O_NONBLOCK)
    {
        return (0);
    }

    switch (errno)
    {
#if defined EAGAIN
    case EAGAIN:
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        return (1);
    }

    return (0);
}

/*
 * Initialize a context
 */
void mbedtls_net_init(mbedtls_net_context *ctx)
{
    ctx->MBEDTLS_PRIVATE(fd) = -1;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free(mbedtls_net_context *ctx)
{
    if (ctx->MBEDTLS_PRIVATE(fd) == -1)
    {
        return;
    }

    shutdown(ctx->MBEDTLS_PRIVATE(fd), 2);
    close(ctx->MBEDTLS_PRIVATE(fd));

    ctx->MBEDTLS_PRIVATE(fd) = -1;
}


/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int fd = ((mbedtls_net_context *) ctx)->MBEDTLS_PRIVATE(fd);

    ret = check_fd(fd, 0);
    if (ret != 0)
    {
        return ret;
    }

    ret = (int) read(fd, buf, len);

    if (ret < 0)
    {
        if (net_would_block(ctx) != 0)
        {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
        !defined(EFI32)
        if (WSAGetLastError() == WSAECONNRESET)
        {
            return MBEDTLS_ERR_NET_CONN_RESET;
        }
#else
        if (errno == EPIPE || errno == ECONNRESET)
        {
            return MBEDTLS_ERR_NET_CONN_RESET;
        }

        if (errno == EINTR)
        {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
#endif

        return MBEDTLS_ERR_NET_RECV_FAILED;
    }

    return ret;
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf,
                             size_t len, uint32_t timeout)
{
    DBG_DIRECT(" ------ mbedtls_net_recv_timeout ------- ");
    return 0;
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int fd = ((mbedtls_net_context *) ctx)->MBEDTLS_PRIVATE(fd);

    ret = check_fd(fd, 0);
    if (ret != 0)
    {
        return ret;
    }

    ret = (int) write(fd, buf, len);

    if (ret < 0)
    {
        if (net_would_block(ctx) != 0)
        {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }

#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
        !defined(EFI32)
        if (WSAGetLastError() == WSAECONNRESET)
        {
            return MBEDTLS_ERR_NET_CONN_RESET;
        }
#else
        if (errno == EPIPE || errno == ECONNRESET)
        {
            return MBEDTLS_ERR_NET_CONN_RESET;
        }

        if (errno == EINTR)
        {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }
#endif

        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    return ret;
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto)
{
    int ret;
    struct addrinfo hints;
    struct addrinfo *list;
    struct addrinfo *current;

    /* Do name resolution with both IPv6 and IPv4 */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

    if (getaddrinfo(host, port, &hints, &list) != 0)
    {
        mbedtls_printf("mbedtls_net_connect(), MBEDTLS_ERR_NET_UNKNOWN_HOST");
        return MBEDTLS_ERR_NET_UNKNOWN_HOST;
    }

    /* Try the sockaddrs until a connection succeeds */
    ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
    for (current = list; current != NULL; current = current->ai_next)
    {
        ctx->MBEDTLS_PRIVATE(fd) = (int) socket(current->ai_family, current->ai_socktype,
                                                current->ai_protocol);
        if (ctx->MBEDTLS_PRIVATE(fd) < 0)
        {
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }

        struct sockaddr_in *temp = (struct sockaddr_in *)current->ai_addr;
        mbedtls_printf("port %d, addr %s", (uint16_t)lwip_ntohs(temp->sin_port), inet_ntoa(temp->sin_addr));
        if (connect(ctx->MBEDTLS_PRIVATE(fd), current->ai_addr, (uint32_t)current->ai_addrlen) == 0)
        {
            ret = 0;
            break;
        }

        close(ctx->MBEDTLS_PRIVATE(fd));
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    }

    freeaddrinfo(list);

    return ret;

}
