/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/timeouts.h"
#include "lwip/prot/ethernet.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/debug.h"
#include "lwip/prot/dhcp.h"
#include "bnepif.h"
#include <string.h>
#include <trace.h>
//#include "os_mem.h"


/*============================================================================*
 *                              Local Variables
 *============================================================================*/
/* Network interface name */
#define IFNAME0 'r'
#define IFNAME1 't'




static struct
{
    struct netif netif;
    BNEPIF_INPUT output;
    uint8_t local_addr[6];
    uint8_t remote_addr[6];
} bnepif =
{
    .netif = {},
};


static err_t bnepif_low_level_output(struct netif *netif, struct pbuf *p);


// helper functions to hide NO_SYS vs. FreeRTOS implementations


/// lwIP functions

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static void bnep_lwip_free_pbuf(struct pbuf *p)
{
#if NO_SYS
    // release buffer / decrease refcount
    pbuf_free(p);
#else
    // release buffer / decrease refcount
    pbuf_free_callback(p);
#endif
}



/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */

static err_t bnep_netif_init(struct netif *netif)
{

    // interface short name
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    // set mac address
    bnepif.netif.hwaddr_len = 6;
    for (uint32_t i = 0; i < 6; i++)
    {
        bnepif.netif.hwaddr[i] = bnepif.local_addr[6 - i - 1];
    }

    uint8_t *addr = bnepif.netif.hwaddr;

    LWIP_PLATFORM_DIAG(("bnep_netif_init: hwaddr %02x:%02x:%02x:%02x:%02x:%02x",
                        addr[0], addr[1], addr[2], addr[3], addr[44], addr[5]));

    // mtu
    netif->mtu = BNEP_MTU;

    /* device capabilities */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...)
     */
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif

    netif->linkoutput = bnepif_low_level_output;

    return ERR_OK;
}

int bnepif_netif_up(uint8_t remote_addr[6])
{
    memcpy(bnepif.remote_addr, remote_addr, 6);
#if LWIP_IPV6
    netif_create_ip6_linklocal_address(&bnepif.netif, 0);
    LWIP_PLATFORM_DIAG(("bnepif_netif_up: remote_addr %02x:%02x:%02x:%02x:%02x:%02x local_addr %02x:%02x:%02x:%02x:%02x:%02x, ipv6 addr %s",
                        remote_addr[5], remote_addr[4], remote_addr[3], remote_addr[2], remote_addr[1], remote_addr[0],
                        bnepif.local_addr[5], bnepif.local_addr[4], bnepif.local_addr[3], bnepif.local_addr[2],
                        bnepif.local_addr[1], bnepif.local_addr[0],
                        ip6addr_ntoa(ip_2_ip6(bnepif.netif.ip6_addr))));
#endif
    ;
    // link is up
    bnepif.netif.flags |= NETIF_FLAG_LINK_UP;

    // if up
    netif_set_up(&bnepif.netif);

    return 0;
}

/**
 * @brief Bring up network interfacd
 * @param network_address
 * @return 0 if ok
 */
int bnepif_netif_down(void)
{
    LWIP_PLATFORM_DIAG(("bnepif_netif_down"));

    // link is down
    bnepif.netif.flags &= ~NETIF_FLAG_LINK_UP;

    netif_set_down(&bnepif.netif);
    return 0;
}

/**
 * @brief Forward packet to TCP/IP stack
 * @param packet
 * @param size
 */
void bnepif_low_level_input(const uint8_t *packet, uint16_t size)
{

    /* We allocate a pbuf chain of pbufs from the pool. */
    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    LWIP_PLATFORM_DIAG(("bnepif_low_level_input, pbuf_alloc = %p,  p->len %d, size %d", p, p->len,
                        size));

    if (!p) { return; }

    /* store packet in pbuf chain */
    struct pbuf *q = p;
    while (q != NULL && size)
    {
        memcpy(q->payload, packet, q->len);
        packet += q->len;
        size   -= q->len;
        q = q->next;
    }

    if (size != 0)
    {
        LWIP_PLATFORM_DIAG(("failed to copy data into pbuf"));
        bnep_lwip_free_pbuf(p);
        return;
    }

    /* pass all packets to ethernet_input, which decides what packets it supports */
    int res = bnepif.netif.input(p, &bnepif.netif);
    if (res != ERR_OK)
    {
        LWIP_PLATFORM_DIAG(("bnep_lwip_netif_process_packet: IP input error\n"));
        bnep_lwip_free_pbuf(p);
        p = NULL;
    }
}


static err_t bnepif_low_level_output(struct netif *netif, struct pbuf *p)
{

    LWIP_PLATFORM_DIAG(("bnepif_low_level_output: packet %p, len %u, total len %u ", p, p->len,
                        p->tot_len));

    uint32_t tot_len = p->tot_len;
    uint8_t *buf = malloc(tot_len);
    if (buf)
    {
        pbuf_copy_partial(p, buf, p->tot_len, 0);
        bnepif.output(bnepif.remote_addr, (uint8_t *) buf, tot_len);
        free(buf);
    }

    return ERR_OK;
}

void bnepif_dhcp_start(void)
{
    dhcp_start(&bnepif.netif);
}

/******************************************************************
 * @brief  init TCPIP
 * @param  none
 * @return none
 */
void bnepif_init(uint8_t local_addr[6], BNEPIF_INPUT output)
{
    memcpy(bnepif.local_addr, local_addr, 6);
    bnepif.output = output;
    ip4_addr_t fsl_netif0_ipaddr = {}, fsl_netif0_netmask = {}, fsl_netif0_gw = {};
    // when using DHCP Client, no address
    IP4_ADDR(&fsl_netif0_ipaddr, 0U, 0U, 0U, 0U);
    IP4_ADDR(&fsl_netif0_netmask, 0U, 0U, 0U, 0U);
    IP4_ADDR(&fsl_netif0_gw, 0U, 0U, 0U, 0U);

    // input function differs for sys vs nosys
    netif_input_fn input_function;
#if NO_SYS
    input_function = ethernet_input;
#else
    input_function = tcpip_input;
#endif

    LWIP_PLATFORM_DIAG(("bnepif_init: netif %p ", &bnepif.netif));

    netif_add(&bnepif.netif, &fsl_netif0_ipaddr, &fsl_netif0_netmask, &fsl_netif0_gw, NULL,
              bnep_netif_init, input_function);
    netif_set_default(&bnepif.netif);
}
