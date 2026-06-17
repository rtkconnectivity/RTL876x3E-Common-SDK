/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef _HID_OVER_ISO_DEF_H_
#define _HID_OVER_ISO_DEF_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

/* Add Includes here */
#include <stdint.h>
#include <stdbool.h>

/*============================================================================*
 *                         Macros
 *============================================================================*/
/** @defgroup HID_OVER_ISO_DEF_Exported_Macros HID Over ISO Service Exported Macros
  * @brief
  * @{
  */

/** @defgroup HID_OVER_ISO_SVC_UUID
  * @brief HID over ISO service UUID
  * @{
  */
#define GATT_UUID_HID_OVER_ISO                    0xFF01 /**< For testing only */
#define GATT_UUID_CHAR_HID_OVER_ISO_PROPERTIES    0xFF02 /**< For testing only */
#define GATT_UUID_CHAR_LE_HID_OPERATION_MODE      0xFF03 /**< For testing only */
/** @} End of HID_OVER_ISO_SVC_UUID */

#define ATT_ERR_HID_OVER_ISO_OPCODE_OUTSIDE_RANGE              (ATT_ERR | 0x81)
#define ATT_ERR_HID_OVER_ISO_DEVICE_ALREADY_IN_REQUESTED_STATE (ATT_ERR | 0x82)
#define ATT_ERR_HID_OVER_ISO_UNSUPPORTED_FEATURE               (ATT_ERR | 0x83)

#define HID_OVER_ISO_PROPERTIES_CHAR_FEATURES_BIT_VALUE_DEVICE_MODE_CHANGE_SUPPORED 0x01

#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_1_MS    0x0001
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_2_MS    0x0002
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_3_MS    0x0004
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_4_MS    0x0008
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_5_MS    0x0010
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_1_25_MS 0x0020
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_2_5_MS  0x0040
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_3_75_MS 0x0080
#define HID_OVER_ISO_PROPERTIES_CHAR_SUPPORTED_REPORT_INTERVALS_BIT_VALUE_7_5_MS  0x0100

#define HID_OVER_ISO_PROPERTIES_CHAR_HYBRID_MODE_ISO_REPORTS_BIT_VALUE_OUTPUT_REPORT          0x01
#define HID_OVER_ISO_PROPERTIES_CHAR_HYBRID_MODE_ISO_REPORTS_BIT_VALUE_CONFIRMATION_SUPPORTED 0x02
#define HID_OVER_ISO_PROPERTIES_CHAR_HYBRID_MODE_ISO_REPORTS_BIT_VALUE_REPETITION_SUPPORTED   0x04

#define LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_HYBRID_OPERATION_MODE  0x01
#define LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_DEFAULT_OPERATION_MODE 0x02
#define LE_HID_OPERATION_MODE_CHAR_OPCODE_MAX  LE_HID_OPERATION_MODE_CHAR_OPCODE_SELECT_DEFAULT_OPERATION_MODE


#define HYBRID_MODE_ISO_REPORTS_NUM_MIN 1
#define HYBRID_MODE_ISO_REPORTS_NUM_MAX 8
/** End of HID_OVER_ISO_DEF_Exported_Macros
* @}
*/

/*============================================================================*
 *                         Types
 *============================================================================*/
/** @defgroup HID_OVER_ISO_DEF_Exported_Types HID Over ISO Service Exported Types
  * @brief
  * @{
  */

typedef struct
{
    uint8_t     report_id;
    uint8_t     additional_info;
} T_HYBRID_MODE_ISO_REPORTS;

/** @brief HID over ISO Properties characteristic format*/
typedef struct
{
    uint8_t     features;
    uint16_t    supported_report_intervals;
    uint8_t     max_sdu_size_device_to_host;
    uint8_t     max_sdu_size_host_to_device;
    uint8_t     reports_num;
    T_HYBRID_MODE_ISO_REPORTS reports[HYBRID_MODE_ISO_REPORTS_NUM_MAX];
} T_HID_OVER_ISO_PROPERTIES_VALUE;

typedef struct
{
    uint8_t report_info_index: 3;
    uint8_t rfu: 3;
    uint8_t confirmation_enable: 1;
    uint8_t repetition_enable: 1;
} T_HYBRID_MODE_ISO_REPORTS_ENABLE_SUB_FIELD;

typedef struct
{
    uint8_t     cig_id;
    uint8_t     cis_id;
    uint16_t    report_interval;
    uint8_t     reports_num;
    T_HYBRID_MODE_ISO_REPORTS_ENABLE_SUB_FIELD
    hybrid_mode_iso_reports_enable[HYBRID_MODE_ISO_REPORTS_NUM_MAX];
} T_SELECT_HYBRID_OPERATION_MODE_PARAM;

/** @brief LE HID Operation Mode characteristic format*/
typedef struct
{
    uint8_t     opcode;
    union
    {
        T_SELECT_HYBRID_OPERATION_MODE_PARAM hybrid_param;
    } param;
} T_LE_HID_OPERATION_MODE_VALUE;

/** @brief HID over ISO packet structure
 * A HID over ISO packet can contain either a HID report or a Confirmation.
*/
typedef struct
{
    uint8_t length;
    uint8_t sequence_number;
    uint8_t report_id;
} T_HID_OVER_ISO_PACKET;

/** End of HID_OVER_ISO_DEF_Exported_Types
* @}
*/


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif  /* _HID_OVER_ISO_DEF_H_ */
