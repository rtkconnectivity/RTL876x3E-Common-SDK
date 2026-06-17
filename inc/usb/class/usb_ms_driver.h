/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef __USB_MS_DRIVER_H__
#define __USB_MS_DRIVER_H__
#include <stddef.h>
#include <stdbool.h>
#include "usb_spec20.h"
#include "usb_msc_bot.h"

/**
 * \addtogroup USB_MS_Driver
 * \brief The module mainly provides components to implement the USB mass storage class.
 * This driver only supports Bulk-Only transport, and does not concern which command set
 * specification and storage medium are used.
 * @{
  */

/**
 * \defgroup USB_MS_DRIVER_USAGE How to Implement USB Mass Storage Interface
 * @{
 *
 * \brief This section provides a comprehensive guide on implementing a USB mass storage interface,
 *        complete with sample code for your reference.
 *
 * \section USB_MSC_DRIVER_INITIALIZE_MSC_INTERFACE Initialize MSC Interface
 * Implement descriptor arrays of the USB MSC interface.
 * - The Bulk endpoints are utilized exclusively for command, data, and status transport.
 * - The default pipe is only used for tasks such as clearing a STALL condition on Bulk endpoints and for issuing class-specific requests as defined by the spec.
 * - There is no requirement for an Interrupt endpoint in this specification.
 *
 * \par Example
 * \code
 *      #include "usb_spec20.h"
 *
 *      T_USB_INTERFACE_DESC demo_ms_if_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC demo_ms_bo_ep_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC demo_ms_bi_ep_desc_fs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_INTERFACE_DESC demo_ms_if_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC demo_ms_bo_ep_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_ENDPOINT_DESC demo_ms_bi_ep_desc_hs =
 *      {
 *          ...init...
 *      };
 *
 *      T_USB_DESC_HDR *demo_ms_descs_fs[] =
 *      {
 *          (T_USB_DESC_HDR*)&demo_ms_if_desc_fs,
 *          (T_USB_DESC_HDR*)&demo_ms_bo_ep_desc_fs,
 *          (T_USB_DESC_HDR*)&demo_ms_bi_ep_desc_fs,
 *          NULL
 *      };
 *
 *      T_USB_DESC_HDR *demo_ms_descs_hs[] =
 *      {
 *          (T_USB_DESC_HDR*)&demo_ms_if_desc_fs,
 *          (T_USB_DESC_HDR*)&demo_ms_bo_ep_desc_fs,
 *          (T_USB_DESC_HDR*)&demo_ms_bi_ep_desc_fs,
 *          NULL
 *      };
 *
 *      usb_ms_driver_if_desc_register(demo_ms_descs_fs, demo_ms_descs_hs);
 * \endcode
 *
 * \section USB_MSC_DRIVER_REGISTER_CALLBACKS_TO_DISK_DRIVER Register Callbacks to Disk Driver
 * Register SD card operations to the MSC driver.
 *
 * \par Example
 * \code
 *      int demo_usb_ms_disk_format(void)
 *      {
 *          ...
 *      }
 *
 *      int demo_usb_ms_disk_read(uint32_t lba, uint32_t blk_num, uint8_t *data)
 *      {
 *          ...
 *      }
 *
 *      int demo_usb_ms_disk_write(uint32_t lba, uint32_t blk_num, uint8_t *data)
 *      {
 *          ...
 *      }
 *
 *      bool demo_usb_ms_disk_is_ready(void)
 *      {
 *          ...
 *      }
 *
 *      int demo_usb_ms_disk_remove(void)
 *      {
 *          ...
 *      }
 *
 *      int usb_ms_disk_capacity_get(uint32_t *max_lba, uint32_t *blk_len)
 *      {
 *          ...
 *      }
 *
 *      T_DISK_DRIVER demo_usb_ms_disk_driver =
 *      {
 *          .type = 0,
 *          .format = demo_usb_ms_disk_format,
 *          .read = demo_usb_ms_disk_read,
 *          .write = demo_usb_ms_disk_write,
 *          .is_ready = demo_usb_ms_disk_is_ready,
 *          .remove = demo_usb_ms_disk_remove,
 *          .capacity_get = usb_ms_disk_capacity_get
 *      };
 *
 *      usb_ms_driver_disk_register((T_DISK_DRIVER *)&demo_usb_ms_disk_driver);
 * \endcode
 *
 * \section USB_MSC_DRIVER_REGISTER_CALLBACKS_TO_PROCESS_CMD Register Callbacks to Process Command
 * Register callbacks to process command set specification. \n
 * We use SCSI commands and statuses as the command set. The SCSI protocol defines a set of standardized commands
 * and status codes used to perform operations such as reading and writing data, inquiring device status,
 * and managing storage devices.
 *
 * \par Example
 * \code
 *      int demo_usb_ms_cmd_proc(T_DISK_DRIVER *disk, T_CBW *cbw)
 *      {
 *          //process command set such as SCSI command
 *          ...
 *      }
 *
 *      int demo_usb_ms_data_proc(T_DISK_DRIVER *disk, uint8_t *data, uint32_t len)
 *      {
 *          //process data stage
 *          ...
 *      }
 *
 *      T_RX_CB demo_usb_ms_rx_cb =
 *      {
 *          .cmd = demo_usb_ms_cmd_proc,
 *          .data = demo_usb_ms_data_proc
 *      };
 *      usb_ms_driver_data_pipe_ind_cbs_register(&demo_usb_ms_rx_cb);
 * \endcode
 *
 *
 * \section USB_MSC_DRIVER_SET_LOGIC_UNIT_INFO Set Logic Unit Information
 * Set logic unit information defined in \ref T_USB_MS_LOGIC_UNIT_INFO.
 * For SCSI commands set, the information is used in response of the INQUIRY command.
 *
 * \par Example
 * \code
 *      T_USB_MS_LOGIC_UNIT_INFO demo_usb_ms_logic_unit_info =
 *      {
 *          .vendor_id = "Realtek",
 *          .product_id = "USB Mass Storage",
 *          .revision = "1.0"
 *      };
 *      usb_ms_driver_logic_unit_info_set(&demo_usb_ms_logic_unit_info);
 * \endcode
 * @}
  */

/** @defgroup USB_MS_Driver_Exported_Types USB MS Driver Exported Types
  * @{
  */

/**
 * \brief The definitions of disk driver callbacks.
 *
 * This structure defines the interface for a disk driver, including callbacks for
 * various disk operations such as read, write, format, and capacity retrieval.
 *
 * \param type            The disk type.
 *
 * \param format          The callback to format the disk.
 *
 * \param read            The callback to read data from the disk.
 *                        - \b Parameters:
 *                          - \c lba: The logical block address to read from.
 *                          - \c blk_num: The number of blocks to read starting from lba.
 *                          - \c data: The read data is stored.
 *                        - \b Return: Indicates if the read operation is successful. Refer to `errno.h`.
 *
 * \param write           The callback to write data to the disk.
 *                        - \b Parameters:
 *                          - \c lba: The logical block address to write to.
 *                          - \c blk_num: The number of blocks to write starting from lba.
 *                          - \c data: The data to write.
 *                        - \b Return: Indicates if the write operation is successful. Refer to `errno.h`.
 *
 * \param is_ready        The callback to check if the disk is ready.
 *
 * \param remove          The callback to remove the disk.
 *
 * \param capacity_get    The callback to get the disk capacity.
 *                        - \b Parameters:
 *                          - \c max_lba: The maximum logical block address.
 *                          - \c blk_len: The length of each block.
 *                        - \b Return: Indicates if the capacity retrieval was successful. Refer to `errno.h`.
 */
typedef struct _t_disk_driver
{
    uint8_t type;

    uint32_t blk_size;
    uint32_t max_blk_per_access;

    int (*format)(void);
    int (*read)(uint32_t lba, uint32_t blk_num, uint8_t *data);
    int (*write)(uint32_t lba, uint32_t blk_num, uint8_t *data);
    bool (*is_ready)(void);
    int (*remove)(void);
    int (*capacity_get)(uint32_t *max_lba, uint32_t *blk_len);

    void *(*buffer_alloc)(uint32_t blk_num);
    int (*buffer_free)(void *buf);
} T_DISK_DRIVER;

/**
 * \brief The definition of USB mass storage logic unit information.
 *
 * This structure defines the information of the USB mass storage logic unit, including vendor ID,
 * product ID, and revision.
 *
 * \param vendor_id       The vendor ID of the USB mass storage device.
 * \param product_id      The product ID of the USB mass storage device.
 * \param revision        The revision of the USB mass storage device.
 */
typedef struct _t_usb_ms_logic_unit_info
{
    const char *vendor_id;
    const char *product_id;
    const char *revision;
} T_USB_MS_LOGIC_UNIT_INFO;

/**
 * \brief RX callback structure to process received data.
 *
 * This structure defines callbacks for processing different stages of Bulk-Only Transport (BOT) operations.
 *
 * \param cmd             The callback to process the Command Block Wrapper (CBW) stage of BOT.
 *                        - \b Parameters:
 *                          - \c disk: The disk driver defined in \ref T_DISK_DRIVER.
 *                          - \c cbw: The CBW to process.
 *                        - \b Return: Indicates if the CBW is processed successfully, refer to `errno.h`.
 *
 * \param data            The callback to process the data stage of BOT.
 *                        - \b Parameters:
 *                          - \c disk: The disk driver defined in \ref T_DISK_DRIVER.
 *                          - \c data: The data to process.
 *                          - \c len: The length of the data.
 *                        - \b Return: Indicates if the data is processed successfully, refer to `errno.h`.
 */
typedef struct _t_rx_cb
{
    int (*cmd)(T_DISK_DRIVER *disk, T_CBW *cbw);
    int (*data)(T_DISK_DRIVER *disk, uint8_t *data,
                uint32_t len);
} T_RX_CB;

/**
 * \brief The definition of callback, which will be called when data sent is complete.
 *
 * \param disk          The disk driver defined in \ref T_DISK_DRIVER.
 * \param actual        The actual length of data that has been sent.
 * \param status        Indicate if data is sent successfully.
 * \return Refer to `errno.h`.
 *
 */
typedef int (*USB_MS_DRIVER_TX_COMPLETE)(T_DISK_DRIVER *disk, uint32_t actual, int status);

/** End of group USB_MS_Driver_Exported_Types
  * @}
  */

/** @defgroup USB_MS_Driver_Exported_Functions USB MS Driver Exported Functions
  * @{
  */

/**
* \brief Peek TX buffer.
*
* Ping-pong buffer is used to send data. If length of data to send is large, for example, 4K, \n
* please use this function to peek buffer to store the data to send. \n
*
* \return The TX buffer.
*
* \par Example
* Please refer to \ref USB_MS_DRIVER_USAGE.
*/
uint8_t *usb_ms_driver_tx_buf_peek(void);
/**
 * \brief Send data by bulk in endpoint.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param data          The data to send.
 * \param len           The length of data to send.
 * \param complete      Refer to \ref USB_MS_DRIVER_TX_COMPLETE.
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_data_pipe_send(void *data, uint32_t len, USB_MS_DRIVER_TX_COMPLETE complete);

/**
 * \brief Register callbacks to process received data.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param cbs       Refer to \ref T_RX_CB.
 * \return          Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_data_pipe_ind_cbs_register(T_RX_CB *cbs);

/**
 * \brief Unregister callbacks to process received data.
 *
 * \param cbs       Refer to \ref T_RX_CB.
 * \return          Refer to `errno.h`.
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_data_pipe_ind_cbs_unregister(void);

/**
 * \brief Register disk driver.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param disk      Refer to \ref T_DISK_DRIVER.
 * \return          Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_disk_register(T_DISK_DRIVER *disk);

/**
 * \brief Unregister disk driver.
 *
 * \param disk      Refer to \ref T_DISK_DRIVER.
 * \return          Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_disk_unregister(T_DISK_DRIVER *disk);

/**
 * \brief  Register mass storage interface descriptors.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \param  fs_desc Mass storage interface descriptor of full speed.
 * \param  hs_desc Mass storage interface descriptor of high speed.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_if_desc_register(T_USB_DESC_HDR **descs_fs,  T_USB_DESC_HDR **descs_hs);

/**
 * \brief Unregister mass storage interface descriptors.
 *
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_if_desc_unregister(void);

/**
 * \brief Get logic unit information.
 *
 * \return Refer to \ref T_USB_MS_LOGIC_UNIT_INFO.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
T_USB_MS_LOGIC_UNIT_INFO *usb_ms_driver_logic_unit_info_get(void);
/**
 * \brief Set logic unit information.
 *
 * \param info Refer to \ref T_USB_MS_LOGIC_UNIT_INFO.
 * \return Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_logic_unit_info_set(T_USB_MS_LOGIC_UNIT_INFO *info);
/**
 * \brief Initialize USB mass storage driver.
 *
 * \xrefitem Experimental_Added_API_2_13_0_0 "Experimental Added Since 2.13.0.0" "Experimental Added API"
 *
 * \return          Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_init(void);

/**
 * \brief Deinitialize USB mass storage driver.
 *
 * \return          Refer to `errno.h`.
 *
 * \par Example
 * Please refer to \ref USB_MS_DRIVER_USAGE.
 */
int usb_ms_driver_deinit(void);
/** @} */ /* End of group USB_MS_Driver_Exported_Functions */
/** @}*/
#endif
