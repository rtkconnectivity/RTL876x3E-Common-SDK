/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LEA_FLAGS_H_
#define _APP_LEA_FLAGS_H_

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#define GATTC_TBL_STORAGE_SUPPORT        1
#define BAP_BROADCAST_SOURCE             1
#define BAP_BROADCAST_ASSISTANT          1
#define LE_AUDIO_BIG_INTERLEAVED         1

#define BAP_UNICAST_CLIENT               1
#define CSIP_SET_COORDINATOR             1
#define LE_AUDIO_CIG_INTERLEAVED         1

#define VCP_VOLUME_CONTROLLER            1
#define MICP_MIC_CONTROLLER              1
#define CCP_CALL_CONTROL_SERVER          1
#define MCP_MEDIA_CONTROL_SERVER         1

#define LE_AUDIO_ISO_REF_CLK             1
#endif

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
// CT and UMR must support ASCS and PACS service
#define F_APP_TMAP_CT_SUPPORT           1
#define F_APP_TMAP_UMR_SUPPORT          1
#define F_APP_TMAP_BMR_SUPPORT          1
#define F_APP_TMAS_SUPPORT              1

#define F_APP_LC3_PLUS_SUPPORT          0
#define F_APP_FRAUNHOFER_SUPPORT        0

#if F_APP_TMAP_CT_SUPPORT
#undef F_APP_CCP_SUPPORT
#define F_APP_CCP_SUPPORT               1

#undef F_APP_VCS_SUPPORT
#define F_APP_VCS_SUPPORT               1
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_TMAP_UMR_SUPPORT
#undef F_APP_MCP_SUPPORT
#define F_APP_MCP_SUPPORT               1

#undef F_APP_VCS_SUPPORT
#define F_APP_VCS_SUPPORT               1
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_TMAP_BMR_SUPPORT
#undef F_APP_BASS_SUPPORT
#define F_APP_BASS_SUPPORT              1
#undef F_APP_OPTIONAL_SUPPORT
#define F_APP_OPTIONAL_SUPPORT          1
#endif

#if F_APP_OPTIONAL_SUPPORT
#define F_APP_LEA_CMD_SUPPORT           1
#define GATTC_TBL_STORAGE_SUPPORT       1
#define F_APP_LE_AUDIO_CIS_RND_ADDR     0
#define F_APP_LE_AUDIO_REF_CLK          1
#define F_APP_LE_ISO_REF_CLK            1
#define F_APP_LEA_CSIS_ENC              1
#define F_APP_MICS_SUPPORT              0
#define F_APP_CSIS_SUPPORT              1
#define F_APP_EATT_SUPPORT              0
#endif

#endif

#endif
