/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <stdint.h>
#include "app_link_util_cs.h"
#include "app_device.h"
#include "voice_prompt.h"
#include "remote.h"
#include "app_charger.h"
#include "app_bt_policy_api.h"
#include "app_auto_power_off.h"


#include "app_mmi.h"

#if (BAP_BROADCAST_ASSISTANT || BAP_BROADCAST_SOURCE)
#include "app_lea_ini_bap_bsrc.h"
#endif

#if CCP_CALL_CONTROL_SERVER
#include "app_lea_ini_ccp.h"
#endif

#if MCP_MEDIA_CONTROL_SERVER
#include "app_lea_ini_mcp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** @defgroup APP_MAIN App Main
  * @brief Main entry function for audio sample application.
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Macros App Main Macros
    * @{
    */

/** End of APP_MAIN_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Types App Main Types
    * @{
    */

/**  @brief  App define global app data structure */
typedef struct
{
    T_APP_BR_LINK               br_link[MAX_BR_LINK_NUM];
    T_APP_LE_LINK               le_link[MAX_BLE_LINK_NUM];

    uint16_t                    external_mcu_mtu;

    uint8_t                     local_batt_level;          /**< local battery level */
    uint8_t                     remote_batt_level;         /**< remote battery level */

    uint8_t                     factory_addr[6];            /**< local factory address */
    uint8_t                     avrcp_play_status;
    uint8_t                     a2dp_play_status;

    uint16_t                    max_eq_len;
    uint8_t                     wait_resume_a2dp_idx;
    uint8_t                     update_active_a2dp_idx;

    T_APP_DEVICE_STATE          device_state;
    uint8_t                     remote_session_state;
    bool                        peri_wake_up;
    bool                        reject_call_by_key;                   /*reject incoming by key*/

    uint8_t                     jig_subcmd : 3;
    uint8_t                     jig_dongle_id : 5;
    T_POWER_OFF_CAUSE           power_off_cause;

    T_APP_CHARGER_STATE         local_charger_state;
    uint8_t                     first_hf_index;
    uint8_t                     last_hf_index;

    uint8_t                     sd_playback_switch;
    bool                        hfp_report_batt;

    uint8_t                     bt_addr_disconnect[6];
    bool                        ignore_led_mode;

    bool                        play_min_max_vol_tone;
    bool                        is_circular_vol_up;
    bool                        gaming_mode;
    bool                        playback_muted;
    bool                        voice_muted;
    bool                        power_on_by_cmd;
    bool                        power_off_by_cmd;

    T_APP_TONE_VP_STARTED       tone_vp_status;
    bool                        wait_resume_a2dp_flag;
    bool                        sco_wait_a2dp_sniffing_stop;
    uint8_t                     pending_sco_idx;
    bool                        a2dp_wait_a2dp_sniffing_stop;
    uint8_t                     pending_a2dp_idx;
    uint8_t                     active_media_paused;

    bool                        bt_is_ready;
    uint8_t                     remote_spk_channel;
    bool                        detect_suspend_by_out_ear;
    uint8_t                     b2s_connected_num;
    uint8_t                     rsv6;
    T_MMI_ACTION                key_action_disallow_too_close;

    uint8_t                     adp_high_wake_up_for_zero_box_bat_vol : 1;
    uint8_t                     need_sync_case_battery_to_pri : 1;
    uint8_t                     rsv5 : 6;

    uint8_t                     remote_default_channel;
    T_AUDIO_EFFECT_INSTANCE     apt_eq_instance;

    bool                        remote_is_8753bau;
    bool                        gaming_mode_request_is_received;
    bool                        ignore_bau_first_gaming_cmd;
    bool                        restore_gaming_mode;
    bool                        dongle_is_enable_mic;
    bool                        dongle_is_disable_apt;
    bool                        disallow_play_gaming_mode_vp;
    bool                        ignore_bau_first_gaming_cmd_handled;

    // multilink
    uint8_t                     resume_addr[6]; // reconnected addr after roleswap

    uint8_t                     connected_num_before_roleswap : 2;
    uint8_t                     disallow_connected_tone_after_inactive_connected : 1;
    uint8_t                     rsv7 : 5;

    uint8_t                     pending_mmi_action_in_roleswap;

    uint8_t                     remote_bat_vol;
    bool                        src_roleswitch_fail_no_sniffing;
    bool                        rsv8;
    bool                        force_enter_dut_mode_when_acl_connected;
    bool                        rsv3;
    uint8_t                     spk_eq_mode;
    uint8_t                     rsv9;
    bool                        disallow_sniff;
    bool                        recover_param;

    T_FACTORY_RESET_CLEAR_MODE  factory_reset_clear_mode;
    bool                        waiting_factory_reset;
    uint8_t                     ble_common_adv_after_roleswap;
    uint8_t                     down_count;

    uint8_t                     sw_eq_type;
    uint8_t                     a2dp_preemptive_flag;
    uint16_t                    last_gaming_mode_audio_track_latency;

    uint8_t                     pending_handle_power_off: 1;
    uint8_t                     is_long_press_power_on_play_vp : 1;
    uint8_t                     disconnect_inactive_link_actively : 1;
    uint8_t                     factory_reset_ignore_led_and_vp : 1;
    uint8_t                     ble_is_ready : 1;
    uint8_t                     airplane_bt_shutdown_busy : 1;
    uint8_t                     rsv1 : 1;

    uint8_t                     mic_mp_verify_pri_sel_ori : 3;
    uint8_t                     mic_mp_verify_pri_type_ori : 2;
    uint8_t                     mic_mp_verify_sec_sel_ori : 3;

    uint8_t                     mic_mp_verify_sec_type_ori : 2;
    uint8_t                     ignore_set_pri_audio_eq : 1;
    uint8_t                     eq_ctrl_by_src : 1;
    uint8_t                     last_bud_loc_event : 4;

#if F_APP_LINEIN_SUPPORT
    T_AUDIO_EFFECT_INSTANCE     line_in_eq_instance;
#endif

#if F_APP_SAIYAN_MODE
    uint8_t                     saiyan_org_role;
#endif

    bool                        connected_tone_need;

    /* le audio source */
#if F_APP_LE_AUDIO_INITIATOR_SUPPORT
    uint8_t                     bap_role;
    uint8_t                     cap_role;
    T_OS_QUEUE                  conn_dev_queue;
    T_OS_QUEUE                  group_handle_queue;
    T_OS_QUEUE                  scan_dev_queue;
#if BAP_BROADCAST_ASSISTANT
    T_OS_QUEUE                  sync_handle_queue;
    T_OS_QUEUE                  scan_baas_dev_queue;
#endif
#if (BAP_BROADCAST_ASSISTANT || BAP_BROADCAST_SOURCE)
    T_BROADCAST_SOURCE_CB       bsrc_db;
    uint8_t                     bsrc_state;
#endif
#if MCP_MEDIA_CONTROL_SERVER
    T_APP_LEA_MCP_DB            mcp_db;
#endif
#if CCP_CALL_CONTROL_SERVER
    T_APP_LEA_CCP_DB            ccp_db;
#endif
    bool                        bis_ull_mode;
    bool                        cis_ull_mode;
    T_OS_QUEUE                  iso_input_queue;        //host to controller
    T_OS_QUEUE                  input_path_pending_q;   //host to controller temp
    T_OS_QUEUE                  iso_output_queue;       //controller to host

    bool                        output_left_ready;
    bool                        output_right_ready;
    uint8_t                     output_chnl_mask;
    uint8_t                     output_block_num;
    uint16_t                    output_left_handle;
    uint8_t                     output_left_len;
    T_OS_QUEUE                  output_left_queue;
    uint16_t                    output_right_handle;
    uint8_t                     output_right_len;
    T_OS_QUEUE                  output_right_queue;
    uint32_t                    output_frame_duration;

#endif

    bool                        delay_power_on;
    uint32_t                    playback_device;

} T_APP_DB;
/** End of APP_MAIN_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_MAIN_Exported_Variables App Main Variables
    * @{
    */
extern T_APP_DB app_db;

extern void *audio_evt_queue_handle;
extern void *audio_io_queue_handle;
/** End of APP_MAIN_Exported_Variables
    * @}
    */

/** End of APP_MAIN
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_MAIN_H_ */
