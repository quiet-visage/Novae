#include "db.h"

#include <assert.h>
#include <fieldfusion.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task.h"

#define DB_NAME "resources/db"
#define SQLCMD_GET_STREAK_DATA                                   \
    "WITH "                                                      \
    "Filtered AS("                                               \
    "  SELECT *"                                                 \
    "  FROM task"                                                \
    "  WHERE focus_spent_secs>0"                                 \
    "  group by DATE(date_created)"                              \
    "),"                                                         \
    "StartingPoints AS("                                         \
    "  SELECT *,"                                                \
    "         CASE WHEN "                                        \
    "    julianday(DATE(date_created)) - "                       \
    "julianday(LAG(DATE(date_created)) OVER (ORDER BY "          \
    "date_created))"                                             \
    "    > 1 THEN 1 ELSE 0 END AS IsStart"                       \
    "  FROM Filtered"                                            \
    "), Grouped AS ("                                            \
    "SELECT *, Sum(IsStart)"                                     \
    "	OVER (ORDER BY date_created ROWS UNBOUNDED PRECEDING) AS " \
    "GroupId"                                                    \
    "FROM StartingPoints"                                        \
    ")"                                                          \
    "SELECT MIN(DATE(date_created)) AS StartDate, COUNT(*) AS "  \
    "Count, MAX(DATE(date_created)) AS End_Date"                 \
    "FROM Grouped"                                               \
    "GROUP BY GroupId"                                           \
    "HAVING COUNT(*)>1"                                          \
    "ORDER BY StartDate"

#define SQLCMD_CREATE_TABLE_TASK                                    \
    "CREATE TABLE task ("                                           \
    "task_id INTEGER NOT NULL UNIQUE,"                              \
    "name TEXT NOT NULL,"                                           \
    "date_created DATETIME NOT NULL,"                               \
    "done INT NOT NULL,"                                            \
    "left INT NOT NULL,"                                            \
    "focus_spent_secs FLOAT DEFAULT 0,"                             \
    "rest_spent_secs  FLOAT DEFAULT 0,"                             \
    "date_completed DATETIME,"                                      \
    "total_spent_secs INT AS (focus_spent_secs + rest_spent_secs) " \
    "STORED,"                                                       \
    "PRIMARY KEY (task_id AUTOINCREMENT));"
#define SQLCMD_QUERY_TABLE_NAMES \
    "SELECT name FROM sqlite_master WHERE type='table';"
#define SQLCMD_QUERY_TABLE(TBL) "SELECT * FROM " #TBL ";"
#define SQLCMD_GET_TABLE_INFO(TBL) "PRAGMA table_info(" #TBL ");"
#define SQLCMD_STR_INSERT_CAP 1024

sqlite3* g_db = {0};

typedef enum {
    LOAD_TASK_VAL_FIELD_ID,
    LOAD_TASK_VAL_FIELD_NAME,
    LOAD_TASK_VAL_FIELD_DONE,
    LOAD_TASK_VAL_FIELD_LEFT,
    LOAD_TASK_VAL_FIELD_DATE_COMPLETED,
} Load_Task_Val_Field;

typedef enum {
    ACTIVITY_VAL_FIELD_FOCUS_SPENT_SECS,
    ACTIVITY_VAL_FIELD_REST_SPENT_SECS,
    ACTIVITY_VAL_FIELD_DATE_CREATED,
} Activity_Val_Field;

typedef enum {
    TBL_FIELD_ID,
    TBL_FIELD_NAME,
    TBL_FIELD_DATE_CREATED,
    TBL_FIELD_DONE,
    TBL_FIELD_LEFT,
    TBL_FIELD_FOCUS_TIME_SPENT_SECS,
    TBL_FIELD_REST_TIME_SPENT_SECS,
    TBL_FIELD_DATE_COMPLETED,
    TBL_FIELD_TOTAL_SPENT_SECS,
} Table_Info_Field;

typedef enum {
    STREAK_VAL_FIELD_START_DATE,
    STREAK_VAL_FIELD_END_DATE,
    STREAK_VAL_FIELD_COUNT,
} Streak_Val_Field;

typedef enum {
    TABLE_INFO_KIND_CID = 0,
    TABLE_INFO_KIND_NAME = 1,
    TABLE_INFO_KIND_TYPE = 2,
    TABLE_INFO_KIND_NOTNULL = 3,
    TABLE_INFO_KIND_DFLTVALUE = 4,
    TABLE_INFO_KIND_ISPK = 5,
} Table_Info_Kind_Idx;

static bool is_stage_valid(Table_Info_Field stage, char** col_vals) {
    switch (stage) {
        case TBL_FIELD_ID: {
            return (
                !strcmp(col_vals[TABLE_INFO_KIND_NAME], "task_id") &&
                !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "1") &&
                !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_FIELD_NAME: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "name") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "TEXT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_FIELD_DATE_CREATED: {
            return (
                !strcmp(col_vals[TABLE_INFO_KIND_NAME],
                        "date_created") &&
                !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATETIME") &&
                !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_FIELD_DONE: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "done") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_FIELD_LEFT: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "left") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_FIELD_FOCUS_TIME_SPENT_SECS: {
            return (
                !strcmp(col_vals[TABLE_INFO_KIND_NAME],
                        "focus_spent_secs") &&
                !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "FLOAT") &&
                !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_FIELD_REST_TIME_SPENT_SECS: {
            return (
                !strcmp(col_vals[TABLE_INFO_KIND_NAME],
                        "rest_spent_secs") &&
                !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "FLOAT") &&
                !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_FIELD_TOTAL_SPENT_SECS: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME],
                            "total_spent_secs") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT"));
        } break;
        case TBL_FIELD_DATE_COMPLETED: {
            return (
                !strcmp(col_vals[TABLE_INFO_KIND_NAME],
                        "date_completed") &&
                !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATETIME") &&
                !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "0"));
        } break;
    };
    return 1;
}

static int verify_table_info_cb(void* stage_arg, int col_len,
                                char** col_val, char** col_name) {
    assert(col_len == 6);
    Table_Info_Field* stage = (Table_Info_Field*)stage_arg;
    bool is_valid = is_stage_valid(*stage, col_val);
    if (!is_valid) printf("failed stage: %d", *stage);
    *stage += 1;
    return !is_valid;
}

static void db_exec_cmd(char* cmd,
                        int (*callback)(void*, int, char**, char**),
                        void* cb_arg1) {
    char* err_msg = 0;
    int res = sqlite3_exec(g_db, cmd, callback, cb_arg1, &err_msg);
    if (res == SQLITE_OK) return;
    printf(
        "sql command failed : \n\n%s\n\n--- sqlite exit code: %d "
        "---\n",
        err_msg, res);
    if (err_msg) sqlite3_free(err_msg);
    assert(0 && "sql command failed");
}

static int print_output_cb(void*, int len, char** vals, char** cols) {
    for (size_t i = 0; i < len; i += 1)
        printf("%s: %s\n", cols[i], vals[i]);

    return 0;
}

static bool is_table_task_valid(void) {
    Table_Info_Field stage = 0;
    int err = sqlite3_exec(g_db, SQLCMD_GET_TABLE_INFO(task),
                           verify_table_info_cb, &stage, 0);
    assert(err == SQLITE_OK || err == SQLITE_ABORT);
    return err != SQLITE_ABORT;
}

static bool task_table_exists(void) {
    char* err_msg = 0;
    int res =
        sqlite3_exec(g_db, SQLCMD_QUERY_TABLE(task), 0, 0, &err_msg);
    if (res == SQLITE_OK) {
        assert(!err_msg);
        return 1;
    }
    assert(err_msg);
    char* nst_sub = strstr(err_msg, "no such table");
    assert(nst_sub);
    return 0;
}

void db_init(void) {
    int res = sqlite3_open(DB_NAME, &g_db);
    assert(res == SQLITE_OK);

    // db_exec_cmd("DROP TABLE IF EXISTS task", 0, 0);
    if (!task_table_exists())
        db_exec_cmd(SQLCMD_CREATE_TABLE_TASK, 0, 0);

    if (!is_table_task_valid()) {
        db_exec_cmd("DROP TABLE task", 0, 0);
        db_exec_cmd(SQLCMD_CREATE_TABLE_TASK, 0, 0);
        assert(is_table_task_valid());
    }
}

void db_terminate(void) { sqlite3_close(g_db); }

void db_print_table(void) {
    db_exec_cmd(SQLCMD_QUERY_TABLE(task), print_output_cb, 0);
}

#define DB_CREATE_TASK_CMD_PREFIX \
    "INSERT INTO task (name, date_created, done, left) "
#define DB_CREATE_TASK_CMD_PREFIX_LEN \
    sizeof(DB_CREATE_TASK_CMD_PREFIX)
int db_create_task(const char* name, int done, int left) {
    static char sql_cmd[SQLCMD_STR_INSERT_CAP] =
        DB_CREATE_TASK_CMD_PREFIX;
    char* write_ptr = &sql_cmd[DB_CREATE_TASK_CMD_PREFIX_LEN - 1];
    memset(write_ptr, 0,
           SQLCMD_STR_INSERT_CAP - DB_CREATE_TASK_CMD_PREFIX_LEN);
    snprintf(write_ptr, SQLCMD_STR_INSERT_CAP,
             "VALUES ('%s', datetime('now'), %d, %d)", name, done,
             left);
    db_exec_cmd(sql_cmd, 0, 0);
    int id = sqlite3_last_insert_rowid(g_db);

    return id;
}

const char* get_db_increment_kind_string(DB_Incr_Kind kind) {
    switch (kind) {
        case INCR_TIME_SPENT_FOCUS: return "focus_spent_secs";
        case INCR_TIME_SPENT_REST: return "rest_spent_secs";
    };
    assert(0);
}

void db_incr_time(int id, float delta, DB_Incr_Kind kind) {
#define SQL_CMD_CAP 256
    const char* field = get_db_increment_kind_string(kind);
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = %s + %f FROM (SELECT task_id FROM "
             "task) WHERE "
             "task.task_id = %d;",
             field, field, delta, id);
    db_exec_cmd(sql_cmd, 0, 0);
}

void db_set_completed(int id) {
#define SQL_CMD_CAP 256
    const char* field = "date_completed";
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = datetime('now') FROM (SELECT "
             "task_id FROM task) WHERE "
             "task.task_id = %d;",
             field, id);
    db_exec_cmd(sql_cmd, 0, 0);
}

void db_incr_done(int id) {
#define SQL_CMD_CAP 256
    const char* field = "done";
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = %s + 1 FROM (SELECT "
             "task_id FROM task) WHERE "
             "task.task_id = %d;",
             field, field, id);
    db_exec_cmd(sql_cmd, 0, 0);
}

static int db_get_count_cb(void* out, int len, char** vals,
                           char** cols) {
    if (!*vals) return 0;
    *(size_t*)out = strtoul(*vals, 0, 10);
    return 0;
}

static int db_get_count_f_cb(void* out, int len, char** vals,
                             char** cols) {
    if (!*vals) return 0;
    *(float*)out = strtof(*vals, 0);
    return 0;
}

size_t db_get_todays_task_count(void) {
    size_t count = -1;
    db_exec_cmd(
        "SELECT COUNT(*) FROM task WHERE date_created >= "
        "date('now');",
        db_get_count_cb, &count);
    assert(count != -1);
    return count;
}

static int db_load_todays_task_cb(void* task_ptr_arg, int len,
                                  char** vals, char** cols) {
    Task** task_pp = task_ptr_arg;
    Task* task = *task_pp;
    *task = task_create();

    task->db_id = strtoul(vals[LOAD_TASK_VAL_FIELD_ID], 0, 10);

    task->name_len = strlen(vals[LOAD_TASK_VAL_FIELD_NAME]);
    C32 name32[task->name_len + 8];
    ff_utf8_to_utf32(name32, vals[LOAD_TASK_VAL_FIELD_NAME],
                     task->name_len);
    task_set_name(task, name32, task->name_len);

    task->done = strtoul(vals[LOAD_TASK_VAL_FIELD_DONE], 0, 10);
    task->left = strtoul(vals[LOAD_TASK_VAL_FIELD_LEFT], 0, 10);
    task->complete = vals[LOAD_TASK_VAL_FIELD_DATE_COMPLETED];

    *task_pp += 1;
    return 0;
}

void db_get_todays_task(Task* out, size_t cap) {
    db_exec_cmd(
        "SELECT task_id, name, done, left, date_completed FROM task "
        "WHERE "
        "date_created >= "
        "date('now');",
        db_load_todays_task_cb, &out);
}

void db_set_done(int id, int val) {
#define SQL_CMD_CAP 256
    const char* field = "done";
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = %d FROM (SELECT "
             "task_id FROM task) WHERE "
             "task.task_id = %d;",
             field, val, id);
    db_exec_cmd(sql_cmd, 0, 0);
}

size_t db_get_all_time_activity_count(void) {
    size_t count = 0;
    db_exec_cmd(
        "SELECT DISTINCT COUNT() OVER() FROM task WHERE "
        "focus_spent_secs > 0 GROUP BY DATE(date_created)",
        db_get_count_cb, &count);
    return count;
}

static Date sql_date_str_to_date(char* str) {
    char* year_beg = &str[0];
    char* month_beg = &str[5];
    char* day_beg = &str[8];
    Date result = {
        .year = strtoul(year_beg, 0, 10),
        .month = strtoul(month_beg, 0, 10) - 1,
        .day = strtoul(day_beg, 0, 10),
    };
    return result;
}

static int get_time_activity_cb(void* arg1, int len, char** vals,
                                char** cols) {
    assert(len == 3);
    Time_Activity** tas_pp = arg1;
    Time_Activity* tas = *tas_pp;

    tas->activity.focus =
        strtof(vals[ACTIVITY_VAL_FIELD_FOCUS_SPENT_SECS], 0);
    tas->activity.rest =
        strtof(vals[ACTIVITY_VAL_FIELD_REST_SPENT_SECS], 0);
    tas->date =
        sql_date_str_to_date(vals[ACTIVITY_VAL_FIELD_DATE_CREATED]);
    *tas_pp += 1;
    return 0;
}

void db_get_all_time_activity(Time_Activity* tas) {
    db_exec_cmd(
        "SELECT SUM(focus_spent_secs) AS focus_spent_secs, "
        "SUM(rest_spent_secs) AS rest_spent_secs, DATE(date_created) "
        "AS date_created FROM task GROUP BY DATE(date_created);",
        get_time_activity_cb, &tas);
}

static int db_get_streak_data_cb(void* arg, int len, char** vals,
                                 char** cols) {
    Streak_Data** streak_array_ptr = arg;
    Streak_Data* result = *streak_array_ptr;

    result->start_date =
        sql_date_str_to_date(vals[STREAK_VAL_FIELD_START_DATE]);
    result->end_date =
        sql_date_str_to_date(vals[STREAK_VAL_FIELD_END_DATE]);
    result->streak_count =
        strtoul(vals[STREAK_VAL_FIELD_COUNT], 0, 10);
    *streak_array_ptr += 1;

    return 0;
}

size_t db_get_streak_data_count(void) {
    char* sql_cmd =
        "SELECT COUNT(*) FROM (" SQLCMD_GET_STREAK_DATA ")";
    size_t result = 0;
    db_exec_cmd(sql_cmd, db_get_count_cb, &result);
    return result;
}

void db_get_streak_data(Streak_Data* out) {
    db_exec_cmd(SQLCMD_GET_STREAK_DATA, db_get_streak_data_cb, &out);
}

int db_get_today_activity_cb(void* arg, int len, char** val,
                             char** col) {
    if (!*val) return 0;
    assert(len == 2);
    Activity* out = arg;
    out->focus = strtof(val[0], 0);
    out->rest = strtof(val[1], 0);
    return 0;
}

Activity db_get_today_activity(void) {
    Activity result = {0};
    db_exec_cmd(
        "SELECT SUM(focus_spent_secs) AS focus_spent_secs, "
        "SUM(rest_spent_secs) AS rest_spent_secs "
        "FROM task WHERE DATE(date_created) = DATE('now') "
        "GROUP BY DATE(date_created);",
        db_get_today_activity_cb, &result);
    return result;
}
