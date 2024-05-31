#pragma once

#include <stddef.h>

#include "tag.h"
#include "task.h"

typedef enum {
    INCR_TIME_SPENT_FOCUS,
    INCR_TIME_SPENT_REST,
    INCR_TIME_SPENT_IDLE,
} DB_Incr_Kind;

typedef int Task_Id;
typedef int Tag_Id;

typedef struct {
    float focus;
    float rest;
    float idle;
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

bool db_is_default_task(int id);
void db_init(void);
void db_terminate(void);

Tag_Id db_create_tag(const char* name, unsigned color);
void db_delete_tag(int id);
size_t db_get_tag_count(void);
void db_get_tags(Tag* out);  // allocates memory for tag.name, user needs to free it

void db_print_table(void);
Task_Id db_create_task(const char* name, int done, int left);
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
size_t db_get_streak(void);
void db_batch_incr_time(int id, float delta, DB_Incr_Kind kind);
void db_batch_flush(void);
