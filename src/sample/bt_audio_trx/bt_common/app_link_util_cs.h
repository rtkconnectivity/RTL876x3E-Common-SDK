/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_LINK_UTIL_CS_H_
#define _APP_LINK_UTIL_CS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_hfp.h"
#include "tts.h"
#include "btm.h"
#include "codec_qos.h"
#include "os_queue.h"
#include "audio_type.h"
#include "app_types.h"
#include "app_avrcp.h"
#include "app_eq.h"
#include "app_flags.h"
#include "app_hfp_ag.h"
#include "app_hfp_types.h"

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
#include "app_lea_acc_ascs_def.h"
#include "app_lea_acc_ccp_def.h"
#include "app_lea_acc_def.h"
#endif

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
#include "gap_conn_le.h"
#include "app_lea_ini_conn_mgr.h"
#if BAP_BROADCAST_ASSISTANT
#include "broadcast_source_sm.h"
#include "ble_audio_sync.h"
#endif
#endif

#if CONFIG_REALTEK_GFPS_FEATURE_SUPPORT
#include "app_gfps.h"
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_lea_ini_conn_mgr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_LINK App Link
  * @brief App Link
  * @{
  */

/** bitmask of profiles */
#define A2DP_SINK_PROFILE_MASK          0x00000001    /**< A2DP SINK profile bitmask */
#define AVRCP_CONTROLLER_PROFILE_MASK   0x00000002    /**< AVRCP CONTROLLER profile bitmask */
#define HFP_HF_PROFILE_MASK             0x00000004    /**< HFP HF profile bitmask */
#define RDTP_PROFILE_MASK               0x00000008    /**< Remote Control vendor profile bitmask */
#define SPP_PROFILE_MASK                0x00000010    /**< SPP profile bitmask */
#define IAP_PROFILE_MASK                0x00000020    /**< iAP profile bitmask */
#define PBAP_PCE_PROFILE_MASK           0x00000040    /**< PBAP PCE profile bitmask */
#define HSP_AG_PROFILE_MASK             0x00000080    /**< HSP AG profile bitmask */
#define HID_PROFILE_MASK                0x00000100    /**< HID profile bitmask */
#define MAP_MSE_PROFILE_MASK            0x00000200    /**< MAP MSE profile bitmask */
#define A2DP_SRC_PROFILE_MASK           0x00000400    /**< A2DP SRC profile bitmask */
#define AVRCP_TARGET_PROFILE_MASK       0x00000800    /**< AVRCP TARGET profile bitmask */
#define HFP_AG_PROFILE_MASK             0x00001000    /**< HFP AG profile bitmask */
#define PBAP_PSE_PROFILE_MASK           0x00002000    /**< PBAP PSE profile bitmask */
#define HSP_HF_PROFILE_MASK             0x00004000    /**< HSP HF profile bitmask */
#define MAP_MCE_PROFILE_MASK            0x00008000    /**< MAP MCE profile bitmask */
#define GATT_PROFILE_MASK               0x00010000    /**< GATT profile bitmask */
#define GFPS_PROFILE_MASK               0x00020000    /**< GFPS profile bitmask */
#define XIAOAI_PROFILE_MASK             0x00040000    /**< XIAOAI profile bitmask */
#define AMA_PROFILE_MASK                0x00080000    /**< AMA profile bitmask */
#define AVP_PROFILE_MASK                0x00100000    /**< AVP profile bitmask */
#define DID_PROFILE_MASK                0x80000000    /**< DID profile bitmask */

#if F_APP_A2DP_SOURCE_SUPPORT
#define A2DP_PROFILE_MASK               A2DP_SINK_PROFILE_MASK
#define AVRCP_PROFILE_MASK              AVRCP_CONTROLLER_PROFILE_MASK
#elif F_APP_A2DP_SINK_SUPPORT
#define A2DP_PROFILE_MASK               A2DP_SRC_PROFILE_MASK
#define AVRCP_PROFILE_MASK              AVRCP_TARGET_PROFILE_MASK
#endif

#if F_APP_HFP_AG_SUPPORT
#define HFP_PROFILE_MASK                HFP_HF_PROFILE_MASK
#define HSP_PROFILE_MASK                HSP_HF_PROFILE_MASK
#elif F_APP_HFP_HF_SUPPORT
#define HFP_PROFILE_MASK                HFP_AG_PROFILE_MASK
#define HSP_PROFILE_MASK                HSP_AG_PROFILE_MASK
#endif

#if F_APP_BT_PROFILE_PBAP_PCE_SUPPORT
#define PBAP_PROFILE_MASK               PBAP_PSE_PROFILE_MASK
#else
#define PBAP_PROFILE_MASK               PBAP_PCE_PROFILE_MASK
#endif

#if F_APP_BT_PROFILE_MAP_MCE_SUPPORT
#define MAP_PROFILE_MASK                MAP_MSE_PROFILE_MASK
#else
#define MAP_PROFILE_MASK                MAP_MCE_PROFILE_MASK
#endif

#define ALL_PROFILE_MASK                0xffffffff

#define MAX_ADV_SET_NUMBER              6

/**  @brief iap data session status */
typedef enum
{
    DATA_SESSION_CLOSE = 0x00,
    DATA_SESSION_LAUNCH = 0x01,
    DATA_SESSION_OPEN = 0x02,
} T_DATA_SESSION_STATUS;

typedef enum
{
    APP_HF_STATE_STANDBY = 0x00,
    APP_HF_STATE_CONNECTED = 0x01,
} T_APP_HF_STATE;

typedef enum
{
    APP_REMOTE_DEVICE_UNKNOWN = 0x00,
    APP_REMOTE_DEVICE_IOS     = 0x01,
    APP_REMOTE_DEVICE_OTHERS  = 0x02,
} T_APP_REMOTE_DEVICE_VEDDOR_ID;

/**  @brief  APP's Bluetooth BR/EDR link connection database */

typedef struct T_APP_BR_LINK
{
    uint8_t             bd_addr[6];
    bool                used;
    uint8_t             id;

    struct
    {
        bool                resume;
        bool                enable;
        uint8_t            *buf;
        uint16_t            len;
        uint8_t             tx_seqn;
        uint8_t             rx_seqn;
        uint8_t             tx_mask;
        struct
        {
            uint8_t            *rx_dt_pkt;
            uint16_t            rx_dt_pkt_len;
        } uart;
    } cmd;

    struct
    {
        bool                audio_eq_enabled;
        T_AUDIO_EQ_INFO     audio_set_eq_info;
        T_AUDIO_EQ_INFO     audio_get_eq_info;
        T_AUDIO_EFFECT_INSTANCE instance;
        T_AUDIO_EFFECT_INSTANCE voice_mic_eq_instance;
#if F_APP_VOICE_SPK_EQ_SUPPORT
        T_AUDIO_EFFECT_INSTANCE voice_spk_eq_instance;
#endif

    } eq;

    T_AUDIO_EFFECT_INSTANCE nrec_instance;

    struct
    {
        uint8_t             credit;
        uint8_t             authen_flag;
        uint16_t            frame_size;
        struct
        {
            uint8_t         session_status;
            uint16_t        session_id;
        } rtk;
    } iap;


    struct
    {
        bool            is_active;
        uint8_t         credit;
        uint16_t        frame_size;
    } vendor_spp;

    struct
    {
        uint8_t         credit;
        uint16_t        frame_size;
    } xm_spp;


    uint32_t            connected_profile;
    uint32_t            plan_disconnect_profs;



    struct
    {
        bool    muted;
        bool    fake_lv0_gain;
        bool    is_streaming;
        void    *track_handle;
        uint8_t stream_only;

        struct
        {
            uint8_t type;
            union
            {
                uint32_t sampling_frequency;
                struct
                {
                    uint32_t sampling_frequency;
                    uint8_t channel_mode;
                    uint8_t block_length;
                    uint8_t subbands;
                    uint8_t allocation_method;
                    uint8_t min_bitpool;
                    uint8_t max_bitpool;
                } sbc;
                struct
                {
                    uint32_t sampling_frequency;
                    uint32_t bit_rate;
                    uint8_t  object_type;
                    uint8_t  channel_number;
                    bool     vbr_supported;
                } aac;
                struct
                {
                    uint8_t sampling_frequency;
                    uint8_t channel_mode;
                } ldac;
                struct
                {
                    uint8_t  info[12];
                } vendor;
            };

        } codec;

    } a2dp;

    struct
    {
        uint8_t             play_status;
        T_APP_ABS_VOL_STATE abs_vol_state;
        uint8_t             vol_set_needed;
    } avrcp;

    struct
    {
        bool                    call_id_type_chk;
        bool                    call_id_type_num;
        bool                    service_status;
        bool                    is_inband_ring;
        T_APP_HF_STATE          state;
        T_APP_HFP_CALL_STATUS   call_status;
        uint16_t                remote_brsf_capability;
    } hfp;

    struct
    {
        T_BT_HFP_AG_CALL_STATUS   call_status;
    } hfp_ag;

    struct
    {
        bool        muted;
        bool        initial;
        bool        duplicate_fst_data;
        uint8_t     seq_num;
        uint16_t    conn_handle;
        void        *track_handle;
    } sco;

    struct
    {
        bool            authed;
        bool            disconnected;
        bool            in_sniffmode;
        bool            decrypted;
        T_BT_LINK_ROLE  role;
        uint16_t        conn_handle;
    } acl;


    uint8_t             pbap_repos;

    uint8_t             *p_gfps_cmd;
    uint16_t            gfps_cmd_len;
    uint8_t             gfps_rfc_chann;

    // 0 not encrpyt; 1 encrypted; 2 de-encrpyted (only for b2b link)
    uint8_t             link_role_switch_count;
    uint16_t            src_conn_idx;
    T_APP_REMOTE_DEVICE_VEDDOR_ID  remote_device_vendor_id;
    uint16_t            remote_device_vendor_version;

    uint8_t             roleswitch_check_after_unsniff_flg: 1;

    uint8_t             b2s_connected_vp_is_played : 1;
    uint8_t             reserved: 2;
    uint16_t            sniff_mode_disable_flag;

    struct
    {
        uint8_t         status;
        uint8_t         reset_num;
    } tpoll;

    uint8_t             rsv[3];
    T_AUDIO_EFFECT_INSTANCE sidetone_instance;
} T_APP_BR_LINK;

typedef void(*P_FUN_LE_LINK_DISC_CB)(uint8_t conn_id, uint8_t local_disc_cause,
                                     uint16_t disc_cause);

typedef struct t_le_disc_cb_entry
{
    struct t_le_disc_cb_entry  *p_next;
    P_FUN_LE_LINK_DISC_CB disc_callback;
} T_LE_DISC_CB_ENTRY;

/**  @brief  App define le link connection state status */
typedef enum
{
    LE_LINK_STATE_DISCONNECTED,
    LE_LINK_STATE_CONNECTING,
    LE_LINK_STATE_CONNECTED,
    LE_LINK_STATE_DISCONNECTING,
} LE_LINK_STATE;

/**  @brief  App define le link encryption state */
typedef enum
{
    LE_LINK_UNENCRYPTIONED = 0,
    LE_LINK_ENCRYPTIONED = 1,
    LE_LINK_ERROR = 2,
} LE_ENCRYPT_STATE;

#define LEA_LINK_DISC_PACS_DONE     0x0001
#define LEA_LINK_DISC_ASCS_DONE     0x0002
#define LEA_LINK_DISC_CSIS_DONE     0x0004
#define LEA_LINK_DISC_BASS_DONE     0x0008
#define LEA_LINK_DISC_CAS_DONE      0x0010

#define APP_LEA_CCID_MAX_NUM 4

#if BAP_BROADCAST_ASSISTANT
typedef struct
{
    bool    brs_is_used;
    uint8_t instance_id;
    uint8_t source_id;
    T_BROADCAST_SOURCE_HANDLE handle;
    T_BLE_AUDIO_SYNC_HANDLE sync_handle;
} T_BASS_BRS_CHAR_DB;
#endif

typedef enum
{
    LE_LOCAL_DISC_CAUSE_UNKNOWN = 0x00,
    LE_LOCAL_DISC_CAUSE_POWER_OFF = 0x01,
    LE_LOCAL_DISC_CAUSE_SWITCH_TO_OTA = 0x02,
    LE_LOCAL_DISC_CAUSE_AUTHEN_FAILED = 0x03,
    LE_LOCAL_DISC_CAUSE_TTS_DISC = 0x04,
    LE_LOCAL_DISC_CAUSE_OTA_RESET = 0x05,
    LE_LOCAL_DISC_CAUSE_ROLESWAP = 0x08,
    LE_LOCAL_DISC_CAUSE_SECONDARY = 0x09,
    LE_LOCAL_DISC_CAUSE_TUYA = 0x0A,
    LE_LOCAL_DISC_CAUSE_ENGAGE_STREAM_PRIOIRTY = 0x0B,
    LE_LOCAL_DISC_CAUSE_CONSOLE_UART = 0x0C,
    LE_LOCAL_DISC_CAUSE_CHARGING_CASE_OTA = 0x0F,
    LE_LOCAL_DISC_CAUSE_CHARGING_CASE_RECONNECT_PRI_BUD = 0x10,
} T_LE_LOCAL_DISC_CAUSE;

/**
 * @brief get remote bd address and address type and store them into p_link.
 *
 * @param p_link  @ref T_APP_LE_LINK
 *
 * remote use RPA(resolvable private address) and resolved address type is public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC;
 * p_link->bd_addr is resloved public address;
 *
 * remote use RPA(resolvable private address) and resolved address type is static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC;
 * p_link->bd_addr is resloved static random address;
 *
 * remote use RPA(resolvable private address) and RPA can not be success resolved:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED;
 * p_link->bd_addr is RPA;
 * Reasons why RPA cannot be successfully resolved may be:
 * 1.Pairing failed or pairing was not performed, resulting in stack unable to obtain the remote identity address.
 * 2.Pairing is successful and stack can obtain the remote identity address, but RPA resolution fails.
 *
 * remote use static random address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM;
 * p_link->bd_addr is static random address;
 *
 * remote use public address:
 * p_link->bd_type is APP_LE_REMOTE_BD_TYPE_PUBLIC;
 * p_link->bd_addr is public address;
 */
typedef enum
{
    APP_LE_REMOTE_BD_TYPE_PUBLIC                       = 0x00,
    APP_LE_REMOTE_BD_TYPE_STATIC_RANDOM                = 0x01,
    APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_PUBLIC = 0x02,
    APP_LE_REMOTE_BD_TYPE_RPA_AND_RESOLVED_TYPE_STATIC = 0x03,
    APP_LE_REMOTE_BD_TYPE_RPA_AND_UNRESOLVED           = 0x04,
} T_APP_LE_REMOTE_BD_TYPE;

/**  @brief  App define le link connection database */
typedef struct
{
    uint8_t                 bd_addr[6];
    T_APP_LE_REMOTE_BD_TYPE bd_type;
    bool                    used;
    uint8_t                 id;

    struct
    {
        bool                enable;
        uint8_t             *buf;
        uint16_t            len;
        uint8_t             rx_seqn;
        uint8_t             tx_seqn;
        uint8_t             tx_mask;
    } cmd;

    T_AUDIO_EQ_INFO         audio_set_eq_info;
    T_AUDIO_EQ_INFO         audio_get_eq_info;

    uint16_t                mtu_size;
    uint16_t                conn_handle;

    LE_LINK_STATE           state;
    uint8_t                 conn_id;


    T_OS_QUEUE              disc_cb_list;


    T_LE_LOCAL_DISC_CAUSE   local_disc_cause;
    LE_ENCRYPT_STATE        encryption_status;

#if CONFIG_REALTEK_BT_GATT_CLIENT_SUPPORT
    bool                    mtu_received;
    bool                    auth_cmpl;
    bool                    start_discover;
#endif

#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
    uint16_t                lea_disc_flags;
    uint16_t                lea_srv_found_flags;
    T_GAP_ROLE              role;
    T_LEA_CONN_PARAM        conn_param;
    uint8_t                 lea_conn_num;
#if BAP_BROADCAST_ASSISTANT
    uint8_t                 brs_char_num;
    T_BASS_BRS_CHAR_DB      *brs_char_tbl;
#endif
#endif

#if F_APP_LE_AUDIO_ACCEPTOR_SUPPORT
    uint8_t                 pre_media_state;
    uint8_t                 media_state;
    T_LEA_ASE_ENTRY         lea_ase_entry[ASCS_ASE_ENTRY_NUM];
    bool                    gmcs;
    uint8_t                 *active_call_uri;
    T_LEA_CALL_ENTRY        lea_call_entry[CCP_CALL_ENTRY_NUM];
    T_APP_CALL_STATUS       call_status;
    bool                    gtbs;
    uint8_t                 active_call_index;

    // Workaround: Use PACS CCCD info as le link connecting, wait le connected to handle lea connect event
    uint8_t                 lea_device;
    T_LEA_LINK_STATE        lea_link_state;
    uint16_t                sink_available_contexts;
    uint16_t                source_available_contexts;
    uint16_t                cis_left_ch_conn_handle;
    uint16_t                cis_right_ch_conn_handle;
    uint8_t                 cis_left_ch_iso_state;
    uint8_t                 cis_right_ch_iso_state;
    uint8_t                 stream_channel_allocation;
#endif

#if F_APP_VCS_SUPPORT
    uint8_t                 volume_setting;
    uint8_t                 mute;
#endif

#if TRANSMIT_CLIENT_SUPPORT
    uint8_t                tx_event_seqn;
#endif

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    uint8_t                bas_client_instance_num;
    uint8_t                dis_client_instance_num;
    bool                   ci_update_cmpl;
    bool                   authen_check;
    uint32_t               vendor_procedure_check;
    bool                   uwb_vendor_update_cmpl;
    uint16_t               uwb_conn_value_update;
    uint16_t               uwb_latency_update;
    uint16_t               uwb_timeout_update;
    uint16_t               uwb_conn_len_update;
    bool                     yylx_offset_update_cmpl;
    bool                   yylx_ci_value_cmpl;
#if F_APP_BT_AUDIO_TRI_DONGLE_LEAUDIO_UNICAST
    uint16_t               ble_device_type;
#endif
#endif
} T_APP_LE_LINK;

uint32_t app_connected_profiles(void);

/**
    * @brief  get BR/EDR link num wich connected with phone by the mask profile
    * @param  profile_mask the mask profile
    * @return BR/EDR link num
    */
uint8_t app_connected_profile_link_num(uint32_t profile_mask);

/**
    * @brief  get current SCO connected link number
    * @param  void
    * @return SCO connected number
    */
uint8_t app_link_get_sco_conn_num(void);

/**
    * @brief  get current a2dp start number
    * @param  void
    * @return a2dp start number
    */
uint8_t app_link_get_a2dp_start_num(void);

/**
    * @brief  get the BR/EDR link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_link_find_br_link(uint8_t *bd_addr);

/**
    * @brief  find the BR/EDR link by connected id
    * @param  conn_id BR/EDR link id(slot)
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_link_find_br_link_by_link_id(uint8_t link_id);

T_APP_BR_LINK *app_link_find_br_link_by_conn_handle(uint16_t conn_handle);

/**
    * @brief  get the BR/EDR link by tts handle
    * @param  handle tts handle
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_link_find_br_link_by_tts_handle(T_TTS_HANDLE handle);

/**
    * @brief  alloc the BR/EDR link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BR/EDR link
    */
T_APP_BR_LINK *app_link_alloc_br_link(uint8_t *bd_addr);

/**
    * @brief  free the BR/EDR link
    * @param  p_link the BR/EDR link
    * @return true: success; false: fail
    */
bool app_link_free_br_link(T_APP_BR_LINK *p_link);

/**
    * @brief  find the BLE link by connected id
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  find the BLE link by link id
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_link_id(uint8_t link_id);

/**
    * @brief  find the BLE link by connected handle
    * @param  conn_handle BLE connection handle
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_conn_handle(uint16_t conn_handle);

/**
    * @brief  find the BLE link by bluetooth address
    * @param  bd_addr bluetooth address
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_find_le_link_by_addr(uint8_t *bd_addr);



/**
    * @brief  alloc the BLE link by link id(slot)
    * @param  conn_id BLE link id(slot)
    * @return the BLE link
    */
T_APP_LE_LINK *app_link_alloc_le_link_by_conn_id(uint8_t conn_id);

/**
    * @brief  free the BLE link
    * @param  p_link the BLE link
    * @return true: success; false: fail
    */
bool app_link_free_le_link(T_APP_LE_LINK *p_link);

bool app_link_reg_le_link_disc_cb(uint8_t conn_id, P_FUN_LE_LINK_DISC_CB p_fun_cb);

uint8_t app_link_get_le_link_num(void);
uint8_t app_link_get_le_encrypted_link_num(void);

bool app_link_check_b2s_link_by_id(uint8_t id);

T_APP_BR_LINK *app_link_find_b2s_link(uint8_t *bd_addr);

/**
    * @brief  get the bud2phone link num
    * @param  void
    * @return link num
    */
uint8_t app_link_get_b2s_link_num(void);

/** End of APP_LINK
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_LINK_UTIL_CS_H_ */
