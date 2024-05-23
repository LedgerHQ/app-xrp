/*******************************************************************************
 *   XRP Wallet
 *   (c) 2020 Towo Labs
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <limits.h>

#include "os.h"

#include "time.h"
#include "readers.h"
#include "fmt.h"
#include "limitations.h"

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH             (946684800LL + 86400 * (31 + 29))
#define RIPPLE_EPOCH_OFFSET 946684800LL
#define FINAL_EPOCH_OFFSET  (RIPPLE_EPOCH_OFFSET - LEAPOCH)

#define DAYS_PER_400Y (365 * 400 + 97)
#define DAYS_PER_100Y (365 * 100 + 24)
#define DAYS_PER_4Y   (365 * 4 + 1)

typedef struct {
    int tm_sec;  /* seconds after the minute [0-60] */
    int tm_min;  /* minutes after the hour [0-59] */
    int tm_hour; /* hours since midnight [0-23] */
    int tm_mday; /* day of the month [1-31] */
    int tm_mon;  /* months since January [0-11] */
    int tm_year; /* years since 1900 */
} tm_mini_t;

bool is_time(field_t *field) {
    if (field->data_type == STI_UINT32) {
        switch (field->id) {
            case XRP_UINT32_EXPIRATION:
            case XRP_UINT32_CANCEL_AFTER:
            case XRP_UINT32_FINISH_AFTER:
                return true;
            default:
                return false;
        }
    } else {
        return false;
    }
}

bool is_time_delta(field_t *field) {
    return field->data_type == STI_UINT32 && field->id == XRP_UINT32_SETTLE_DELAY;
}

// Inspired from http://git.musl-libc.org/cgit/musl/tree/src/time/__secs_to_tm.c?h=v0.9.15
static int ripple_epoch_to_tm(long long t, tm_mini_t *tm) {
    long long days, secs;
    int remdays, remsecs, remyears;
    int qc_cycles, c_cycles, q_cycles;
    int years, months;
    static const char days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

    /* Reject time_t values whose year would overflow int */
    if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL) return -1;

    secs = t + FINAL_EPOCH_OFFSET;
    days = secs / 86400;
    remsecs = secs % 86400;
    if (remsecs < 0) {
        remsecs += 86400;
        days--;
    }

    qc_cycles = days / DAYS_PER_400Y;
    remdays = days % DAYS_PER_400Y;
    if (remdays < 0) {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / DAYS_PER_100Y;
    if (c_cycles == 4) c_cycles--;
    remdays -= c_cycles * DAYS_PER_100Y;

    q_cycles = remdays / DAYS_PER_4Y;
    if (q_cycles == 25) q_cycles--;
    remdays -= q_cycles * DAYS_PER_4Y;

    remyears = remdays / 365;
    if (remyears == 4) remyears--;
    remdays -= remyears * 365;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

    for (months = 0; days_in_month[months] <= remdays; months++) remdays -= days_in_month[months];

    if (years + 100 > INT_MAX || years + 100 < INT_MIN) return -1;

    tm->tm_year = years + 100;
    tm->tm_mon = months + 2;
    if (tm->tm_mon >= 12) {
        tm->tm_mon -= 12;
        tm->tm_year++;
    }
    tm->tm_mday = remdays + 1;

    tm->tm_hour = remsecs / 3600;
    tm->tm_min = remsecs / 60 % 60;
    tm->tm_sec = remsecs % 60;

    return 0;
}

static void print_time(tm_mini_t *tm, field_value_t *dst) {
    snprintf(dst->buf,
             sizeof(dst->buf),
             "%u-%02u-%02u %02u:%02u:%02u UTC",
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec);
}

void format_time(field_t *field, field_value_t *dst) {
    uint32_t value = field->data.u32;

    tm_mini_t tm;
    ripple_epoch_to_tm(value, &tm);

    print_time(&tm, dst);
}

void format_time_delta(field_t *field, field_value_t *dst) {
    uint32_t value = field->data.u32;
    snprintf(dst->buf, sizeof(dst->buf), "%u s", value);
}
