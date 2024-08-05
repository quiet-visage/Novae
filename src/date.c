#include "date.h"

#include <assert.h>
#include <string.h>
#include <time.h>

inline int date_cmp(Date a, Date b) {
    size_t cnp_a = a.day + (a.month * a.month * a.month) + (a.year * a.year * a.year);
    size_t cnp_b = b.day + (b.month * b.month * b.month) + (b.year * b.year * b.year);
    return cnp_a > cnp_b ? 1 : cnp_a < cnp_b ? -1 : 0;
}

Week_Day get_week_day_from_time_str(char* str) {
    assert(strlen(str) >= 3);
    if (str[0] == 'M') return DAY_MON;
    if (str[0] == 'T' && str[1] == 'u') return DAY_TUE;
    if (str[0] == 'W') return DAY_WED;
    if (str[0] == 'T') return DAY_THU;
    if (str[0] == 'F') return DAY_FRI;
    if (str[0] == 'S' && str[1] == 'a') return DAY_SAT;
    if (str[0] == 'S') return DAY_SUN;
    assert(0);
}

Week_Day get_first_month_weekday(int year, Month month) {
    struct tm tm = {0};
    tm.tm_year = year;
    tm.tm_mon = month;
    tm.tm_mday = 0;
    time_t time = mktime(&tm);
    char* time_str = ctime(&time);
    return get_week_day_from_time_str(time_str);
}

int get_number_of_days(int year, Month month) {
    switch (month) {
        case MONTH_JAN: return 31;
        case MONTH_FEB: return IS_LEAP_YEAR(year) ? 29 : 28;
        case MONTH_MAR: return 31;
        case MONTH_APR: return 30;
        case MONTH_MAY: return 31;
        case MONTH_JUN: return 30;
        case MONTH_JUL: return 31;
        case MONTH_AUG: return 31;
        case MONTH_SEP: return 30;
        case MONTH_OCT: return 31;
        case MONTH_NOV: return 30;
        case MONTH_DEC: return 31;
    }
    assert(0);
}

const char* get_month_name_short(Month month) {
    switch (month) {
        case MONTH_JAN: return "Jan";
        case MONTH_FEB: return "Feb";
        case MONTH_MAR: return "Mar";
        case MONTH_APR: return "Apr";
        case MONTH_MAY: return "May";
        case MONTH_JUN: return "Jun";
        case MONTH_JUL: return "Jul";
        case MONTH_AUG: return "Aug";
        case MONTH_SEP: return "Sep";
        case MONTH_OCT: return "Oct";
        case MONTH_NOV: return "Nov";
        case MONTH_DEC: return "Dec";
    }
    assert(0);
}

const char* get_month_name_full(Month month) {
    switch (month) {
        case MONTH_JAN: return "January";
        case MONTH_FEB: return "February";
        case MONTH_MAR: return "March";
        case MONTH_APR: return "April";
        case MONTH_MAY: return "May";
        case MONTH_JUN: return "June";
        case MONTH_JUL: return "July";
        case MONTH_AUG: return "August";
        case MONTH_SEP: return "September";
        case MONTH_OCT: return "October";
        case MONTH_NOV: return "November";
        case MONTH_DEC: return "December";
    }
    assert(0);
}

const char* get_week_day_name_short(Week_Day wday) {
    switch (wday) {
        case DAY_MON: return "Mo"; break;
        case DAY_TUE: return "Tu"; break;
        case DAY_WED: return "We"; break;
        case DAY_THU: return "Th"; break;
        case DAY_FRI: return "Fr"; break;
        case DAY_SAT: return "Sa"; break;
        case DAY_SUN: return "Su"; break;
    }
    assert(0);
}

inline Date get_current_date(void) {
    time_t now = time(0);
    struct tm* local_tm = localtime(&now);
    Date date = {.day = local_tm->tm_mday, .month = local_tm->tm_mon, .year = local_tm->tm_year + 1900};
    return date;
}

void date_incr_month(Date* m) {
    if (m->month < 11)
        ++m->month;
    else {
        ++m->year;
        m->month = 0;
    }
}

void date_decr_month(Date* m) {
    if (m->month)
        --m->month;
    else {
        --m->year;
        m->month = 11;
    }
}

time_t date_to_time(Date date) {
    struct tm tm_info = {0};
    tm_info.tm_mday = date.day;
    tm_info.tm_mon = date.month;
    tm_info.tm_year = date.year - 1900;
    return mktime(&tm_info);
}

long day_diffenrence(Date date_end, Date date_begin) {
    time_t time_end = date_to_time(date_end);
    time_t time_begin = date_to_time(date_begin);
    double seconds = difftime(time_end, time_begin);
    return seconds * 86400;
}
