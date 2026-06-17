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
#include "lwip/prot/dhcp.h"
#include "ethernetif.h"
#include <string.h>
#include <trace.h>
#include "os_mem.h"
#include "app_section.h"
#include "rtl_pinmux.h"
#include "wifi_nic_intf.h"
#include "FreeRTOS.h"
#include "event_groups.h"
/*============================================================================*
 *                              Local Variables
 *============================================================================*/
/* Network interface name */
#define IFNAME0 'r'
#define IFNAME1 't'

#define MAX_ETH_DRV_SG  32
#define MAX_ETH_MSG 1500
struct ethernetif
{
    struct eth_addr *ethaddr;
    /* Add whatever per-interface state that is needed here. */
};

struct netif xnetif[NET_IF_NUM];
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

/*============================================================================*
 *                              Global Variables
 *============================================================================*/
xSemaphoreHandle s_xSemaphore = NULL;

/*============================================================================*
 *                              Functions Declaration
 *============================================================================*/
static err_t low_level_output(struct netif *netif, struct pbuf *p) RAM_FUNCTION;
static struct pbuf *low_level_input(struct netif *netif) RAM_FUNCTION;
void ethernetif_input(void *pParams) RAM_FUNCTION;

/*============================================================================*
 *                              Local Functions
 *============================================================================*/
/******************************************************************
 * @brief  init hardware. Called from ethernetif_init.
 * @param  netif - the already initialized lwip network interface structure for this ethernetif
 * @return none
 */
static void low_level_init(struct netif *netif)
{
    unsigned int mac[6] = {0};

    /* Set netif link flag */
    netif_set_link_up(netif);

#if LWIP_ARP || LWIP_ETHERNET
    /* set MAC hardware address length */
    netif->hwaddr_len = ETH_HWADDR_LEN;

    /*get mac*/
    char mac_str[20] = {0};

    int res = wifi_nic_get_mac_address(mac_str);
    if (res)
    {
        LWIP_PLATFORM_DIAG(("ERROR: wifi_nic_get_mac_address() failed"));
    }
    else
    {
        sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        /* set MAC hardware address */
        netif->hwaddr[0] =  mac[0];
        netif->hwaddr[1] =  mac[1];
        netif->hwaddr[2] =  mac[2];
        netif->hwaddr[3] =  mac[3];
        netif->hwaddr[4] =  mac[4];
        netif->hwaddr[5] =  mac[5];

        LWIP_PLATFORM_DIAG(("low_level_init() MAC Addr= %x:%x:%x:%x:%x:%x", \
                            netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], \
                            netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]));
    }
    /* maximum transfer unit */
    netif->mtu = NETIF_MTU;

#if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
    netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */

#endif /* LWIP_ARP || LWIP_ETHERNET */
}

/******************************************************************
 * @brief  send packet
 * @param  netif - the lwip network interface structure for this ethernetif
 * @param  p - the MAC packet to send
 * @return an err_t value if the packet couldn't be sent or ERR_OK if the packet could be sent
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    /* initialize the one struct eth_drv_sg array */
    T_ETH_DRV_SG sg_list[MAX_ETH_DRV_SG];
    int sg_len = 0;
    struct pbuf *q;
    if (!wifi_nic_is_up())
    {
        return ERR_IF;
    }
    /* packet is stored in one list composed by several pbuf. */
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next)
    {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }
    if (q != NULL)
    {
        LWIP_PLATFORM_DIAG(("low_level_output() q != NULL"));
    }
    if (sg_len)
    {
        wifi_nic_wlan_send(wifi_nic_netif_get_idx(netif), sg_list, sg_len, p->tot_len);
    }
    return ERR_OK;
}

/******************************************************************
 * @brief  receive packet
 * @param  netif - the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet
 */


static struct pbuf *low_level_input(struct netif *netif)
{
    LWIP_PLATFORM_DIAG(("----- run in low_level_input -----"));
    return 0;
}
void lwip_PRE_SLEEP_PROCESSING(void)
{
    LWIP_PLATFORM_DIAG(("----- run in lwip_PRE_SLEEP_PROCESSING -----"));
}
void lwip_POST_SLEEP_PROCESSING(void)
{
    LWIP_PLATFORM_DIAG(("----- run in lwip_POST_SLEEP_PROCESSING -----"));
}

/******************************************************************
 * @brief  receive packet
 * @param  pParams - the lwip network interface structure for this ethernetif
 * @return none
 */
void ethernetif_recv(struct netif *netif, int total_len)
{
//  APP_PRINT_INFO0("----- run in ethernetif_recv -----");
    int errcode;
    T_ETH_DRV_SG sg_list[MAX_ETH_DRV_SG];
    struct pbuf *p, *q;
    int sg_len = 0;

    // WIFI chip is running
    if (!wifi_nic_is_up())
    {
        return;
    }
    if ((total_len > MAX_ETH_MSG) || (total_len < 0))
    {
        total_len = MAX_ETH_MSG;
    }

    // Allocate buffer to store received packet
    p = pbuf_alloc(PBUF_RAW, total_len, PBUF_RAM);

    if (p == NULL)
    {
        LWIP_PLATFORM_DIAG(("Cannot allocate pbuf to receive packet,l%d.", __LINE__));
        return;
    }

    // Create scatter list
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next)
    {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }
//
    // Copy received packet to scatter list from wrapper RX skb
    wifi_nic_wlan_recv(wifi_nic_netif_get_idx(netif), sg_list, sg_len);

    // Pass received packet to the interface
    errcode = netif->input(p, netif); // tcpip_input()
    if (ERR_OK != errcode)
    {
        LWIP_PLATFORM_DIAG(("netif->input error.code=%d.", errcode));
        pbuf_free(p);
    }
}

#if !LWIP_ARP
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
    err_t errval;
    errval = ERR_OK;

    return errval;

}
#endif /* LWIP_ARP */

/*============================================================================*
*                              Global Functions
*============================================================================*/
/******************************************************************
 * @brief  init ethernetif
 * @param  netif - the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized or ERR_MEM if private data couldn't be allocated
 */
err_t ethernetif_init(struct netif *netif)
{
//    APP_PRINT_INFO0("----- run in ethernetif_init -----");
    struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethernetif = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(struct ethernetif));

    if (ethernetif == NULL)
    {
        LWIP_PLATFORM_DIAG(("ethernetif_init() out of memory\n"));
        return ERR_MEM;
    }

    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */
    netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
    netif->output = etharp_output;
#else
    netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);
    ethernetif->ethaddr = (struct eth_addr *) & (netif->hwaddr[0]);

    return ERR_OK;
}

/******************************************************************
 * @brief  init TCPIP
 * @param  none
 * @return none
 */
void TCPIP_Init(netif_callback_t *netif_cb)
{
    tcpip_init(NULL, NULL);

#if LWIP_DHCP
    ip_addr_set_zero_ip4(&ipaddr);
    ip_addr_set_zero_ip4(&netmask);
    ip_addr_set_zero_ip4(&gw);
#else
    IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

    netif_add(&xnetif[0], &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    netif_set_status_callback(&xnetif[0], netif_cb->status_callback);
    netif_set_link_callback(&xnetif[0], netif_cb->link_callback);

    netif_set_default(&xnetif[0]);

    netif_set_up(&xnetif[0]);
}
