/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _SPP_DEMO_APP_H_
#define _SPP_DEMO_APP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void spp_demo_app_init(void);

void spp_demo_app_tx_data(uint8_t *bd_addr, uint8_t *buf, uint8_t len);

void spp_demo_app_connect_req(uint8_t *bd_addr);

void spp_demo_app_disconnect_all_req(uint8_t *bd_addr);

void spp_demo_app_disconnect_req(uint8_t *bd_addr, uint8_t local_server_chann);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SPP_DEMO_APP_H_ */
