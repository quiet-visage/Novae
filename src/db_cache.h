#pragma once
#include "db.h"

void db_cache_init(void);
void db_cache_auto_sync(void);
float db_cache_get_max_diligence(void);
size_t db_cache_get_streak(void);
Activity* db_cache_get_activity(Date date);
size_t db_cache_get_activity_count(void);
Time_Activity* db_cache_get_activity_array(void);
void db_cache_terminate(void);
Activity* db_cache_get_todays_activity(void);
Tag* db_cache_get_tag(int id);
Tag* db_cache_get_default_tag(void);
void db_cache_sync_tags(void);
Tag* db_cache_get_tag_array(void);
size_t db_cache_get_tag_array_len(void);
size_t db_cache_get_future_tasks_len(void);
Task* db_cache_get_future_tasks_array(void);
