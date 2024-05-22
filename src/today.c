#include "today.h"

#include <time.h>

#include "db.h"

float g_secs_until_midnight = 0;
Date g_today = {0};

// void today_init(void) {
//   time_t now = time(0);
//   struct tm *now_tm = localtime(&now);
//   g_today.year = now_tm->tm_year+1900;
//   g_today.month = now_tm->tm_mon;
//   g_today.day = now_tm->tm_mday;
//   struct tm g_tomorrow = ;
// }

// void today_sync(void) {

// }

Date today_get_date(void) {
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    return (Date){
        .year = now_local->tm_year + 1900,
        .month = now_local->tm_mon,
        .day = now_local->tm_mday,
    };
}
