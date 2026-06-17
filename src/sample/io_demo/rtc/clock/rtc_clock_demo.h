/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */
#ifndef _RTC_CLOCK_DEMO_H_
#define _RTC_CLOCK_DEMO_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    volatile uint16_t year;    // 2000+
    volatile uint8_t month;    // 0-11
    volatile uint8_t day;      // 0-30
    volatile uint8_t seconds;  // 0-59
    volatile uint8_t minutes;  // 0-59
    volatile uint8_t hour;     // 0-23
} T_UTC_TIME;

typedef enum
{
    MOn  = 0,
    Tues = 1,
    Wed  = 2,
    Thur = 3,
    Fri  = 4,
    Sat  = 5,
    Sun  = 6
} T_DAY_OF_WEEK;

typedef struct
{
    uint32_t seconds    :   6;
    uint32_t minute     :   6;
    uint32_t hours      :   5;
    uint32_t day        :   5;
    uint32_t month      :   4;
    uint32_t year       :   6;
} T_TIME_BIT;

typedef union
{
    uint32_t data;
    T_TIME_BIT time;
} T_TIME_UNION;

typedef void (* pSystemClockCB)(void);

void rtc_clock_start(void);
void rtc_clock_set_system_clock(T_TIME_UNION time);
T_DAY_OF_WEEK rtc_clock_get_day_of_week(uint32_t secTime);//used to calculate day of week
uint32_t rtc_clock_convert_time_to_second(T_TIME_UNION time);
uint8_t rtc_clock_get_system_clock_second(void);
void rtc_clock_demo(void);

#ifdef __cplusplus
}
#endif


#endif //_RTC_CLOCK_DEMO_H_

