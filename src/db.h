#pragma once

#include <stddef.h>

#include "task.h"

typedef enum {
    INCR_TIME_SPENT_FOCUS,
    INCR_TIME_SPENT_REST,
} DB_Incr_Kind;
typedef int Task_Id;

typedef struct {
    float focus;
    float rest;
} Activity;

typedef struct {
    int year;
    int month;
    int day;
} Date;

typedef struct {
    Activity activity;
    Date date;
} Time_Activity;

typedef struct {
    Date start_date;
    Date end_date;
    size_t streak_count;
} Streak_Data;

void db_init(void);
void db_terminate(void);
void db_print_table(void);
Task_Id db_create_task(const char* name, int done, int left);
void db_incr_time(int id, float delta, DB_Incr_Kind kind);
void db_incr_done(int id);
void db_set_completed(int id);
void db_set_done(int id, int val);
size_t db_get_todays_task_count(void);
void db_get_todays_task(Task* out, size_t cap);
size_t db_get_all_time_activity_count(void);
void db_get_all_time_activity(Time_Activity* tas);
Activity db_get_today_activity(void);
size_t db_get_streak_data_count(void);
void db_get_streak_data(Streak_Data* out);
