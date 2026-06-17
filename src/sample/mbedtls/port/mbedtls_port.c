#include "mbedtls_port.h"
#include "mbedtls_config.h"
#include "platform.h"
#include <stdio.h>
#include "trace.h"
#include "entropy_poll.h"
#include "threading.h"


int mbedtls_hardware_poll(void *data,
                          unsigned char *output, size_t len, size_t *olen)
{
#if (CONFIG_SOC_SERIES_RTL8773D == 1 )
    extern uint32_t (*platform_random)(uint32_t max);
#else
    extern uint32_t platform_random(uint32_t max);
#endif

    uint32_t i;
    uint8_t *output_data = (uint8_t *)output;
    for (i = 0; i < len; i++)
    {
        output_data[i] = platform_random(0xFF);
    }
    *olen = len;
    return 0;
}

#if !CONFIG_REALTEK_RTL87X3G_ALIPAY
#include "os_sync.h"
static void mbedtls_port_mutex_init(mbedtls_threading_mutex_t *mutex)
{
    if (mutex != NULL)
    {
        os_mutex_create(&(mutex->mutex));
    }
}

static void mbedtls_port_mutex_free(mbedtls_threading_mutex_t *mutex)
{
    if (mutex != NULL && mutex->mutex != NULL)
    {
        os_mutex_delete(mutex->mutex);
    }
}


static int mbedtls_port_mutex_lock(mbedtls_threading_mutex_t *mutex)
{
    if (mutex == NULL || mutex->mutex == NULL)
    {
        return MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;
    }
    if (os_mutex_take(mutex->mutex, 0xFFFFFFFF) == true)
    {
        return 0;
    }
    else
    {
        return MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
}


static int mbedtls_port_mutex_unlock(mbedtls_threading_mutex_t *mutex)
{
    if (mutex == NULL || mutex->mutex == NULL)
    {
        return MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;
    }
    if (os_mutex_give(mutex->mutex) == true)
    {
        return 0;
    }
    else
    {
        return MBEDTLS_ERR_THREADING_MUTEX_ERROR;
    }
}

void mbedtls_init(void)
{
//      mbedtls_platform_set_printf(MBEDTLS_PRINT_INFO);
    mbedtls_platform_set_snprintf(snprintf);
    mbedtls_platform_set_vsnprintf(vsnprintf);
    mbedtls_threading_set_alt(mbedtls_port_mutex_init, mbedtls_port_mutex_free, mbedtls_port_mutex_lock,
                              mbedtls_port_mutex_unlock);
}
#endif
