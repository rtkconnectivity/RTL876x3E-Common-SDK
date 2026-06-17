/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#if F_APP_USB_AUDIO_SUPPORT
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "ring_buffer.h"
#include "app_timer.h"
#include "section.h"
#include "usb_audio.h"
#include "usb_audio_stream.h"
#include "os_queue.h"
#include "app_usb_audio.h"
#include "usb_msg.h"
#include "app_io_msg.h"
#include "os_sync.h"
#include "app_usb.h"
#include "app_ipc.h"
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#include "app_tri_dongle_uac.h"
typedef enum
{
    UAS_TIMER_MONITOR_EMPTY,
} T_USB_AUDIO_TIMER;

static uint8_t timer_idx_monitor_empty = 0;
static uint8_t usb_audio_stream_timer_id = 0;
#define MONITOR_TIMER_INTERVAL_MS    100
#endif

#define UAS_DBG_LOG_EN      0

#if TARGET_RTL8763EWM
#define UAS_OUT_POOL_SIZE       (2 * 1024)
#define UAS_IN_POOL_SIZE        (1 * 1024)
#else
#define UAS_OUT_POOL_SIZE       (1 * 1024)
#define UAS_IN_POOL_SIZE        (3 * 1024)
#endif

#define EVT_PARAM_ARRAY_NUM     0x20

#define USB_ISO_PRINT_LOG_CNT   0x001F

#define DRVTYPE2STREAMTYPE(drv_type) (((drv_type) == 1)?USB_AUDIO_STREAM_TYPE_OUT: \
                                      USB_AUDIO_STREAM_TYPE_IN)
typedef enum
{
    USB_AUDIO_STREAM_EVT_DEACTIVE = 1,
    USB_AUDIO_STREAM_EVT_ACTIVE,
    USB_AUDIO_STREAM_EVT_DATA_XMIT,
    USB_AUDIO_STREAM_EVT_VOL_GET,
    USB_AUDIO_STREAM_EVT_VOL_SET,
    USB_AUDIO_STREAM_EVT_MUTE_SET,
    USB_AUDIO_STREAM_EVT_MUTE_GET,
    USB_AUDIO_STREAM_EVT_RES_SET,
    USB_AUDIO_STREAM_EVT_MAX,
} T_USB_AUDIO_STREAM_EVT;

typedef enum
{
    STREAM_STATE_IDLE,
    STREAM_STATE_INITED,
    STREAM_STATE_ACTIVE,
    STREAM_STATE_XMITING,
} T_AUDIO_STREAM_STATE;

typedef struct
{
    void *pipe;
    uint32_t opt;
} T_UAS_EVT_PARAM;

typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t dir: 1;
        uint32_t evt_type: 7;
        uint32_t pipe: 8;
        uint32_t rsv: 16;
    } u;
} T_USB_AUDIO_STREAM_EVT_INFO;

typedef union
{
    uint32_t d32;
    struct
    {
        uint32_t dir: 1;
        uint32_t ual_id: 7;
        uint32_t pipe: 8;
        uint32_t rsv: 16;
    } u;
} T_USB_AUDIO_STREAM_ID;

typedef struct _ual
{
    struct _ual *p_next;
    uint32_t id;
    void *owner;
    void *handle;
//    T_UAS_FUNC *func;
} T_UAL;

typedef struct _usb_audio_stream
{
    struct _usb_audio_stream *p_next;
    void *handle;
    uint32_t pipe;
    T_STREAM_ATTR attr;
    T_AUDIO_STREAM_STATE state;

    T_UAS_CTRL ctrl;
    T_RING_BUFFER pool;
    uint16_t event_cnt;

    uint8_t ual_total;
    void *active_ual;
    T_OS_QUEUE ual_list;
} T_USB_AUDIO_STREAM;

typedef struct _usb_audio_stream_db
{
    T_OS_QUEUE streams[USB_AUDIO_STREAM_TYPE_MAX];
} T_USB_AUDIO_STREAM_DB;

T_USB_AUDIO_STREAM_DB uas_db;
const uint32_t sample_rate_table[8] = {8000, 16000, 32000, 44100, 48000, 88000, 96000, 192000};

static T_UAS_EVT_PARAM evt_param[EVT_PARAM_ARRAY_NUM];

RAM_TEXT_SECTION
static void *usb_audio_stream_search(T_USB_AUDIO_STREAM_TYPE type, uint32_t label)
{

    T_USB_AUDIO_STREAM *stream = (T_USB_AUDIO_STREAM *)uas_db.streams[type].p_first;

    while (stream)
    {
        if (stream->pipe == label)
        {
            break;
        }
        stream = stream->p_next;
    }

    return stream;
}

RAM_TEXT_SECTION
static void *usb_audio_stream_get_by_id(uint32_t id)
{
    T_USB_AUDIO_STREAM_ID stream_id = {.d32 = id};
#if UAS_DBG_LOG_EN
    USB_PRINT_INFO2("stream_id.u.dir = %d, stream_id.u.pipe = %d", stream_id.u.dir, stream_id.u.pipe);
#endif
    return usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_id.u.dir, stream_id.u.pipe);
}

RAM_TEXT_SECTION
uint32_t usb_audio_stream_get_data_len(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_get_data_count(&(stream->pool));
}

uint32_t usb_audio_stream_data_peek(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_peek(&(stream->pool), len, buf);
}

uint32_t usb_audio_stream_data_flush(uint32_t id, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_remove(&(stream->pool), len);
}

uint32_t usb_audio_stream_data_write(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_write(&(stream->pool), buf, len);
}

RAM_TEXT_SECTION
uint32_t usb_audio_stream_get_remaining_pool_size(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);
    return ring_buffer_get_remaining_space(&(stream->pool)) ;
}

uint32_t usb_audio_stream_data_read(uint32_t id, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);

    return ring_buffer_read(&(stream->pool), len, buf);
}

void usb_audio_stream_buf_clear(uint32_t id)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_get_by_id(id);;
    ring_buffer_clear(&(stream->pool));
}

RAM_TEXT_SECTION
static int usb_audio_stream_pipe_xmit_out(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
#if UAS_DBG_LOG_EN
    APP_PRINT_INFO2("usb_audio_stream_pipe_xmit_out, 0x%x-0x%x", buf, len);
#else
    T_USB_AUDIO_STREAM_TYPE stream_type = USB_AUDIO_STREAM_TYPE_OUT;
    T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search(stream_type, pipe->label);

    if ((stream->event_cnt & USB_ISO_PRINT_LOG_CNT) == 0)
    {
        APP_PRINT_INFO3("usb_audio_stream_pipe_xmit_out, label 0x%x, len 0x%x, cnt:%d", pipe->label, len,
                        stream->event_cnt);
    }
    stream->event_cnt++;
#endif
    return app_usb_audio_data_xmit_out(buf, len, pipe->label);
}

RAM_TEXT_SECTION
static bool usb_audio_stream_pipe_xmit_in(T_USB_AUDIO_PIPES *pipe, void *buf, uint32_t len)
{
    T_USB_AUDIO_STREAM_TYPE stream_type = USB_AUDIO_STREAM_TYPE_IN;
    T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search(stream_type, pipe->label);

    uint16_t data_size = ring_buffer_get_data_count(&(stream->pool));
    uint16_t space_size = ring_buffer_get_remaining_space(&(stream->pool));

#if UAS_DBG_LOG_EN
    APP_PRINT_INFO4("usb_audio_stream_pipe_xmit_in, 0x%x-0x%x-0x%x-0x%x", buf, len, stream,
                    ring_buffer_get_data_count(&(stream->pool)));
#else
    if ((stream->event_cnt & USB_ISO_PRINT_LOG_CNT) == 0)
    {
        APP_PRINT_INFO4("usb_audio_stream_pipe_xmit_in, label 0x%x, len 0x%x, data_size 0x%x, cnt %d",
                        pipe->label, len, data_size, stream->event_cnt);
    }
    stream->event_cnt++;
#endif

    if (ring_buffer_get_data_count(&(stream->pool)) >= len)
    {
        ring_buffer_read(&(stream->pool), len, buf);
        app_usb_audio_us_size_change(data_size, space_size, len);
    }
    else
    {
        memset(buf, 0, len);
        app_usb_audio_us_data_empty();
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
#else
        APP_PRINT_ERROR0("usb_audio_stream_pipe_xmit_in, us ring buf empty!");
#endif
        return false;
    }

    return true;
}

RAM_TEXT_SECTION
static bool trigger_evt(T_USB_AUDIO_PIPES *pipe, uint16_t msg_type, uint32_t param)
{
    static uint8_t toggle_idx = 0;
    T_IO_MSG gpio_msg;

    evt_param[toggle_idx].pipe = pipe;
    evt_param[toggle_idx].opt = param;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_IF_AUDIO, msg_type);
    gpio_msg.u.param = (uint32_t)&evt_param[toggle_idx];
    toggle_idx = (toggle_idx + 1) % EVT_PARAM_ARRAY_NUM;
    return app_io_msg_send(&gpio_msg);
}

static int trigger_evt_ctrl(T_USB_AUDIO_PIPES *pipe, T_USB_AUDIO_CTRL_EVT evt, uint8_t dir,
                            uint32_t param)
{
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt;
    uint8_t evt_type = USB_AUDIO_STREAM_EVT_MAX;

    stream_evt.u.dir = DRVTYPE2STREAMTYPE(dir);
    stream_evt.u.pipe = pipe->label;

    switch (evt)
    {
    case USB_AUDIO_CTRL_EVT_ACTIVATE:
        {
            uint32_t s = os_lock();
            T_USB_AUDIO_STREAM *stream =  usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_evt.u.dir,
                                                                  stream_evt.u.pipe);
            ring_buffer_clear(&stream->pool);
            os_unlock(s);

            evt_type = USB_AUDIO_STREAM_EVT_ACTIVE;
        }
        break;

    case USB_AUDIO_CTRL_EVT_DEACTIVATE:
        {
            evt_type = USB_AUDIO_STREAM_EVT_DEACTIVE;
        }
        break;

    case USB_AUDIO_CTRL_EVT_VOL_SET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_VOL_SET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_VOL_GET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_VOL_GET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_MUTE_GET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_MUTE_GET;
        }
        break;

    case USB_AUDIO_CTRL_EVT_MUTE_SET:
        {
            evt_type = USB_AUDIO_STREAM_EVT_MUTE_SET;
        }
        break;

    default:
        break;
    }
    stream_evt.u.evt_type = evt_type;

    return trigger_evt(pipe, stream_evt.d32, param);
}

#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
static void usb_audio_stream_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_INFO2("usb_audio_stream_timeout_cb: timer_evt %d, param %d",
                    timer_evt, param);
    switch (timer_evt)
    {
    case UAS_TIMER_MONITOR_EMPTY:
        {
            app_stop_timer(&timer_idx_monitor_empty);
            app_tri_dongle_uac_no_ds_set_status(APP_UAC_NO_PKT_DATA_NEED_A2DP_STOP);
        }
        break;

    default:
        break;
    }
}
#endif

void usb_audio_stream_evt_handle(uint8_t evt, uint32_t param)
{
    T_UAS_EVT_PARAM *evt_param = (T_UAS_EVT_PARAM *)param;
    uint32_t pipe_label = ((T_USB_AUDIO_PIPES *)evt_param->pipe)->label;
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt = {.d32 = evt};
    uint8_t evt_type = stream_evt.u.evt_type;
    uint8_t stream_type = stream_evt.u.dir;
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_type,
                                                         pipe_label);
    T_USB_AUDIO_PIPE_ATTR attr = {.content.d32 = evt_param->opt};
    bool handle = true;

    switch (evt_type)
    {
    case USB_AUDIO_STREAM_EVT_ACTIVE:
        {
            attr.dir = stream_type;
            app_usb_audio_active(pipe_label, attr);
            stream->event_cnt = 0;
        }
        break;

    case USB_AUDIO_STREAM_EVT_DEACTIVE:
        {
            app_usb_audio_deactive(pipe_label, stream_type);
        }
        break;

    case USB_AUDIO_STREAM_EVT_DATA_XMIT:
        {
            handle = false;
            app_usb_audio_data_xmit_out_handle(pipe_label);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
            if (!app_tri_dongle_uac_check_ds_only_status())
            {
                app_tri_dongle_uac_no_ds_set_status(APP_UAC_RESUM_PKT_DATA_NEED_A2DP_START);

                app_stop_timer(&timer_idx_monitor_empty);
                app_start_timer(&timer_idx_monitor_empty, "monitor_empty",
                                usb_audio_stream_timer_id, UAS_TIMER_MONITOR_EMPTY, 0, false,
                                MONITOR_TIMER_INTERVAL_MS);
            }
#endif
        }
        break;

    case USB_AUDIO_STREAM_EVT_VOL_GET:
        {
            uint16_t type = attr.content.vol.type;
            if (type == USB_AUDIO_VOL_TYPE_CUR)
            {
                stream->ctrl.vol.cur = attr.content.vol.value;
            }
            else if (type == USB_AUDIO_VOL_TYPE_RANGE)
            {
                stream->ctrl.vol.range = attr.content.vol.value;
            }
            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_VOL_GET, cur:0x%x, range:0x%x", stream->ctrl.vol.cur,
                            stream->ctrl.vol.range);
        }
        break;

    case USB_AUDIO_STREAM_EVT_VOL_SET:
        {
            APP_PRINT_INFO2("USB_AUDIO_STREAM_EVT_VOL_SET, cur:0x%x, range:0x%x", stream->ctrl.vol.cur,
                            stream->ctrl.vol.range);
            uint16_t vol;
            vol = attr.content.vol.value;
            if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
            {
                app_usb_audio_set_spk_vol(pipe_label, vol);
            }
            else if (stream_type == USB_AUDIO_STREAM_TYPE_IN)
            {
                app_usb_audio_set_mic_vol(pipe_label, vol);
            }
            if (attr.content.vol.type == USB_AUDIO_VOL_TYPE_CUR)
            {
                if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
                {
                    uint8_t is_max_volume = (vol - stream->ctrl.vol.range == 0) ? 1 : 0;
                    app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_DS_VOL_SET, &is_max_volume);
                }
            }
            else if (attr.content.vol.type == USB_AUDIO_VOL_TYPE_RES)
            {
                app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_SET_RES, NULL);
            }
        }
        break;

    case USB_AUDIO_STREAM_EVT_MUTE_SET:
        {
            APP_PRINT_INFO1("USB_AUDIO_STREAM_EVT_MUTE_SET 0x%x", attr.content.mute.value);
            uint32_t mute = attr.content.mute.value;
            stream->ctrl.mute = attr.content.mute.value;
            if (stream_type == USB_AUDIO_STREAM_TYPE_OUT)
            {
                app_usb_audio_set_spk_mute(pipe_label, mute);
            }
            else if (stream_type == USB_AUDIO_STREAM_TYPE_IN)
            {
                app_usb_audio_set_mic_vol(pipe_label, mute);
            }
            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_MUTE_CTRL, &mute);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle)
    {
        APP_PRINT_INFO5("usb_audio_stream_evt_handle, evt 0x%x, param 0x%x, label %d, stream 0x%x, opt 0x%x",
                        evt_type, param, pipe_label, stream, evt_param->opt);
    }
}

static int usb_audio_stream_pipe_ctrl(struct _usb_audio_pipes *pipe, T_USB_AUDIO_CTRL_EVT evt,
                                      T_USB_AUDIO_PIPE_ATTR ctrl)
{
    return trigger_evt_ctrl(pipe, evt, ctrl.dir, ctrl.content.d32);
}

static T_USB_AUDIO_PIPES usb_audio_pipe =
{
    .label = USB_AUDIO_STREAM_LABEL_1,
    .ctrl = (USB_AUDIO_PIPE_CB_CTRL)usb_audio_stream_pipe_ctrl,
    .downstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_out,
    .upstream = (USB_AUDIO_PIPE_CB_STREAM)usb_audio_stream_pipe_xmit_in,
};

RAM_TEXT_SECTION
void usb_audio_stream_data_trans_msg(uint32_t label)
{
    T_USB_AUDIO_STREAM_EVT_INFO stream_evt;

    stream_evt.d32 = 0;
    stream_evt.u.dir = USB_AUDIO_STREAM_TYPE_OUT;
    stream_evt.u.evt_type = USB_AUDIO_STREAM_EVT_DATA_XMIT;
    if (label == USB_AUDIO_STREAM_LABEL_1)
    {
        stream_evt.u.pipe = label;
        trigger_evt(&usb_audio_pipe, stream_evt.d32, (uint32_t)label);
    }
}

uint32_t usb_audio_stream_ual_bind(uint8_t stream_type, uint8_t label)
{
    T_USB_AUDIO_STREAM *stream = usb_audio_stream_search((T_USB_AUDIO_STREAM_TYPE)stream_type, label);
    T_UAL *ual_node = NULL;
    T_USB_AUDIO_STREAM_ID id = {.u = {.dir = stream_type, .pipe = label}};
    uint8_t *buf = NULL;
    uint32_t pool_size = (stream_type == USB_AUDIO_STREAM_TYPE_OUT) ? UAS_OUT_POOL_SIZE :
                         UAS_IN_POOL_SIZE;

    if (stream == NULL)
    {
        stream = malloc(sizeof(T_USB_AUDIO_STREAM));
        memset(stream, 0, sizeof(T_USB_AUDIO_STREAM));
        stream->pipe = label;
        buf = (uint8_t *)malloc(pool_size);
        ring_buffer_init(&(stream->pool), buf, pool_size);
        os_queue_in(&uas_db.streams[stream_type], stream);
        os_queue_init(&(stream->ual_list));
    }
    stream->ual_total += 1;
    id.u.ual_id = stream->ual_total;

    ual_node = malloc(sizeof(T_UAL));
    ual_node->p_next = NULL;
    ual_node->handle = NULL;
    ual_node->owner = stream;
    ual_node->id = id.d32;
    os_queue_in(&(stream->ual_list), ual_node);

    APP_PRINT_INFO2("usb_audio_stream_ual_bind, type:0x%x, id:0x%x", stream_type, id.d32);

    return (id.d32);
}

void usb_audio_stream_init(void)
{
    memset(&uas_db, 0, sizeof(uas_db));
    usb_audio_init(&usb_audio_pipe);
#if CONFIG_REALTEK_APP_BT_AUDIO_TRI_DONGLE
    app_timer_reg_cb(usb_audio_stream_timeout_cb, &usb_audio_stream_timer_id);
#endif
}
#endif
