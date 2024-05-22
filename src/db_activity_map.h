#pragma once
#include "db.h"

void db_activity_init(void);
void db_activity_auto_sync(void);
float db_activity_get_max_focus(void);
Activity* db_activity_get(Date date);
size_t db_activity_count(void);
Time_Activity* db_activity_get_array(void);
void db_acitivity_terminate(void);
float db_activity_get_max_focus_spent(void);
