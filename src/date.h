#pragma once

#define IS_LEAP_YEAR(YEAR) (!(YEAR % 4))

typedef enum {
    MONTH_JAN,
    MONTH_FEB,
    MONTH_MAR,
    MONTH_APR,
    MONTH_MAY,
    MONTH_JUN,
    MONTH_JUL,
    MONTH_AUG,
    MONTH_SEP,
    MONTH_OCT,
    MONTH_NOV,
    MONTH_DEC,
} Month;

typedef enum {
    DAY_MON,
    DAY_TUE,
    DAY_WED,
    DAY_THU,
    DAY_FRI,
    DAY_SAT,
    DAY_SUN,
} Week_Day;

typedef struct {
    int year;
    int month;
    int day;
} Date;

Week_Day get_first_month_weekday(int year, Month month);
Week_Day get_week_day_from_time_str(char* str);
int get_number_of_days(int year, Month month);
const char* get_month_name(Month month);
Date get_current_date(void);
