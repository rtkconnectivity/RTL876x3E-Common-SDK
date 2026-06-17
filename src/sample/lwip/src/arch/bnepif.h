#ifndef ETHERNETIF_H__
#define ETHERNETIF_H__

#ifdef  __cplusplus
extern "C"
{
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*
 *                         Macros
 *============================================================================*/

/*============================================================================*
 *                         Types
 *============================================================================*/


typedef bool (*BNEPIF_INPUT)(uint8_t addr[6], uint8_t *data, uint16_t data_len);


/*============================================================================*
*                        Export Global Variables
*============================================================================*/

/*============================================================================*
 *                         Functions
 *============================================================================*/
void bnepif_init(uint8_t *local_addr, BNEPIF_INPUT output);

int bnepif_netif_up(uint8_t *remote_addr);

int bnepif_netif_down(void);

void bnepif_low_level_input(const uint8_t *packet, uint16_t size);

void bnepif_dhcp_start(void);

#ifdef  __cplusplus
}
#endif
#endif /* ETHERNETIF_H */
