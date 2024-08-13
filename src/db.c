#include "db.h"

#include <assert.h>
#include <fieldfusion.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "config.h"
#include "date.h"
#include "str.h"
#include "task.h"

#define DEFAULT_TASK_NAME "\007\007\007"
#define DB_NAME "resources/db"
#define SQLCMD_GET_CURRENT_STREAK                                 \
    " WITH "                                                      \
    " Filtered AS("                                               \
    "   SELECT *"                                                 \
    "   FROM task"                                                \
    "   WHERE diligence>0"                                        \
    "   group by DATE(date_created)"                              \
    " ),"                                                         \
    " StartingPoints AS("                                         \
    "   SELECT *,"                                                \
    "          CASE WHEN "                                        \
    "     julianday(DATE(date_created)) - "                       \
    " julianday(LAG(DATE(date_created)) OVER (ORDER BY "          \
    " date_created))"                                             \
    "     > 1 THEN 1 ELSE 0 END AS IsStart"                       \
    "   FROM Filtered"                                            \
    " ), Grouped AS ("                                            \
    " SELECT *, Sum(IsStart)"                                     \
    " 	OVER (ORDER BY date_created ROWS UNBOUNDED PRECEDING) AS " \
    " GroupId"                                                    \
    " FROM StartingPoints"                                        \
    " )"                                                          \
    " SELECT COUNT() AS Streak"                                   \
    " FROM Grouped"                                               \
    " GROUP BY GroupId"                                           \
    " ORDER BY MAX(DATE(date_created)) DESC"                      \
    " LIMIT 1"

#define SQLCMD_GET_STREAK_DATA                                   \
    "WITH "                                                      \
    "Filtered AS("                                               \
    "  SELECT *"                                                 \
    "  FROM task"                                                \
    "  WHERE diligence>0"                                        \
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

#define SQLCMD_CREATE_TABLE_TAG       \
    "CREATE TABLE tag ("              \
    "tag_id INTEGER NOT NULL UNIQUE," \
    "name TEXT NOT NULL,"             \
    "color INT UNSIGNED NOT NULL,"    \
    "PRIMARY KEY (tag_id AUTOINCREMENT));"

#define SQLCMD_CREATE_TABLE_TASK           \
    "CREATE TABLE task ("                  \
    "task_id INTEGER NOT NULL UNIQUE,"     \
    "name TEXT NOT NULL,"                  \
    "date_created DATETIME NOT NULL,"      \
    "done INT NOT NULL,"                   \
    "left INT NOT NULL,"                   \
    "diligence FLOAT DEFAULT 0,"           \
    "rest  FLOAT DEFAULT 0,"               \
    "idle  FLOAT DEFAULT 0,"               \
    "date_completed DATETIME,"             \
    "tag_id INTEGER NOT NULL,"             \
    "date_from DATE DEFAULT 0,"            \
    "date_to DATE DEFAULT 0,"              \
    "PRIMARY KEY (task_id AUTOINCREMENT)," \
    "FOREIGN KEY (tag_id) REFERENCES tag(tag_id));"

#define SQLCMD_QUERY_TABLE_NAMES "SELECT name FROM sqlite_master WHERE type='table';"
#define SQLCMD_QUERY_TABLE(TBL) "SELECT * FROM " #TBL ";"
#define SQLCMD_GET_TABLE_INFO(TBL) "PRAGMA table_info(" #TBL ");"
#define SQLCMD_STR_INSERT_CAP 1024
#define SQL_CMD_CAP 0x100

typedef enum {
    LOAD_TASK_VAL_FIELD_ID,
    LOAD_TASK_VAL_FIELD_NAME,
    LOAD_TASK_VAL_FIELD_DONE,
    LOAD_TASK_VAL_FIELD_LEFT,
    LOAD_TASK_VAL_FIELD_DATE_COMPLETED,
    LOAD_TASK_VAL_FIELD_TAG_ID,
    LOAD_TASK_VAL_FIELD_DATE_CREATED,
    LOAD_TASK_VAL_FIELD_DATE_FROM,
    LOAD_TASK_VAL_FIELD_DATE_TO,
    LOAD_TASK_VAL_FIELD_DILIGENCE,
    LOAD_TASK_VAL_FIELD_IDLE,
    LOAD_TASK_VAL_FIELD_REST,
} Load_Task_Val_Field;

typedef enum {
    ACTIVITY_VAL_FIELD_DILIGENCE,
    ACTIVITY_VAL_FIELD_REST,
    ACTIVITY_VAL_FIELD_IDLE,
    ACTIVITY_VAL_FIELD_DATE_CREATED,
} Activity_Val_Field;

typedef enum { TAG_VAL_FIELD_ID, TAG_VAL_FIELD_NAME, TAG_VAL_FIELD_COLOR } Tag_Val_Field;

typedef enum {
    TBL_TASK_FIELD_ID,
    TBL_TASK_FIELD_NAME,
    TBL_TASK_FIELD_DATE_CREATED,
    TBL_TASK_FIELD_DONE,
    TBL_TASK_FIELD_LEFT,
    TBL_TASK_FIELD_DILIGENCE,
    TBL_TASK_FIELD_REST,
    TBL_TASK_FIELD_IDLE,
    TBL_TASK_FIELD_DATE_COMPLETED,
    TBL_TASK_FIELD_TAG_FK,
    TBL_TASK_FIELD_DATE_FROM,
    TBL_TASK_FIELD_DATE_TO,
    TBL_TASK_FIELD_COUNT,
} Table_Task_Info_Field;

typedef enum {
    TBL_TAG_FIELD_ID,
    TBL_TAG_FIELD_NAME,
    TBL_TAG_FIELD_COLOR,
} Table_Tag_Info_Field;

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

typedef struct {
    Table_Task_Info_Field stage;
    size_t field_count;
} Verify_Table_Task_Arg;

typedef struct {
    int id;
    float focus_delta;
    float rest_delta;
    float idle_delta;
} Batch_Incr_Time_Item;

typedef struct {
    Batch_Incr_Time_Item *data;
    size_t len;
    size_t cap;
} Batch_Incr_Time;

sqlite3 *g_db = {0};
int g_default_task_id = 0;
Batch_Incr_Time g_batch_incr_time = {0};
Str g_batch_cmd_str = {0};

static Batch_Incr_Time batch_incr_time_create(void) {
    Batch_Incr_Time ret;
    ret.cap = 0x100;
    ret.len = 0;
    ret.data = malloc(ret.cap);
    return ret;
}

static void batch_incr_time_push(Batch_Incr_Time *m, Batch_Incr_Time_Item item) {
    size_t req_len = (m->len + 1) * sizeof(*m->data);
    while (req_len > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }
    m->data[m->len++] = item;
}

static Batch_Incr_Time_Item *batch_incr_time_find(Batch_Incr_Time *m, size_t id) {
    Batch_Incr_Time_Item *item = 0;
    for (size_t i = 0; i < m->len; i += 1) {
        if (m->data[i].id != id) continue;
        item = &m->data[i];
        break;
    }
    if (!item) {
        Batch_Incr_Time_Item empty_item = {0};
        batch_incr_time_push(m, empty_item);
        item = &m->data[m->len - 1];
        item->id = id;
    }
    return item;
}

static void batch_incr_destroy(Batch_Incr_Time *m) {
    assert(m->data);
    free(m->data);
}

static bool is_task_stage_valid(Table_Task_Info_Field stage, char **col_vals) {
    switch (stage) {
        case TBL_TASK_FIELD_ID: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "task_id") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "1") && !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_NAME: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "name") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "TEXT") && !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_DATE_CREATED: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "date_created") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATETIME") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_DONE: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "done") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT") && !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_LEFT: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "left") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT") && !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_DILIGENCE: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "diligence") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") && !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "FLOAT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_TASK_FIELD_REST: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "rest") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "FLOAT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_TASK_FIELD_IDLE: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "idle") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "FLOAT") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_TASK_FIELD_DATE_COMPLETED: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "date_completed") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATETIME") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "0"));
        } break;
        case TBL_TASK_FIELD_TAG_FK: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "tag_id") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "0") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INTEGER") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_NOTNULL], "1"));
        } break;
        case TBL_TASK_FIELD_DATE_FROM: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "date_from") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATE") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
        case TBL_TASK_FIELD_DATE_TO: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "date_to") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "DATE") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_DFLTVALUE], "0"));
        } break;
    };
    return 1;
}

static bool is_tag_stage_valid(Table_Tag_Info_Field stage, char **col_vals) {
    switch (stage) {
        case TBL_TAG_FIELD_ID: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "tag_id") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INTEGER") && !strcmp(col_vals[TABLE_INFO_KIND_ISPK], "1"));
        } break;
        case TBL_TAG_FIELD_NAME: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "name") && !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "TEXT"));
        } break;
        case TBL_TAG_FIELD_COLOR: {
            return (!strcmp(col_vals[TABLE_INFO_KIND_NAME], "color") &&
                    !strcmp(col_vals[TABLE_INFO_KIND_TYPE], "INT UNSIGNED"));
        } break;
    };
    return 1;
}

static int verify_table_task_info_cb(void *void_arg, int col_len, char **col_val, char **col_name) {
    assert(col_len == 6);
    Verify_Table_Task_Arg *arg = void_arg;
    bool is_valid = is_task_stage_valid(arg->stage, col_val);
    if (!is_valid) printf("failed task verification stage: %d", arg->stage);
    arg->stage += 1;
    arg->field_count += 1;
    return !is_valid;
}

static int verify_table_tag_info_cb(void *stage_arg, int col_len, char **col_val, char **col_name) {
    assert(col_len == 6);
    Table_Tag_Info_Field *stage = (Table_Tag_Info_Field *)stage_arg;
    bool is_valid = is_tag_stage_valid(*stage, col_val);
    if (!is_valid) printf("failed tag verification stage: %d", *stage);
    *stage += 1;
    return !is_valid;
}

static void db_exec_cmd(char *cmd, int (*callback)(void *, int, char **, char **), void *cb_arg1) {
    char *err_msg = 0;
    int res = sqlite3_exec(g_db, cmd, callback, cb_arg1, &err_msg);
    if (res == SQLITE_OK) return;
    printf(
        "sql command failed : \n\n%s\n\n--- sqlite exit code: %d "
        "---\n",
        err_msg, res);
    if (err_msg) sqlite3_free(err_msg);
    assert(0 && "sql command failed");
}

static int print_output_cb(void *, int len, char **vals, char **cols) {
    for (size_t i = 0; i < len; i += 1) printf("%s: %s\n", cols[i], vals[i]);

    return 0;
}

static bool is_table_task_valid(void) {
    Verify_Table_Task_Arg arg = {0};
    int err = sqlite3_exec(g_db, SQLCMD_GET_TABLE_INFO(task), verify_table_task_info_cb, &arg, 0);
    assert(err == SQLITE_OK || err == SQLITE_ABORT);

    if (arg.field_count != TBL_TASK_FIELD_COUNT) return 0;

    return err != SQLITE_ABORT;
}

static bool is_table_tag_valid(void) {
    Table_Tag_Info_Field stage = 0;
    int err = sqlite3_exec(g_db, SQLCMD_GET_TABLE_INFO(tag), verify_table_tag_info_cb, &stage, 0);
    assert(err == SQLITE_OK || err == SQLITE_ABORT);
    return err != SQLITE_ABORT;
}

static bool table_exists(const char *name) {
    char cmd[SQL_CMD_CAP] = {0};
    size_t cmd_len = snprintf(cmd, SQL_CMD_CAP, "SELECT * FROM %s;", name);
    assert(cmd_len);

    char *err_msg = 0;
    int res = sqlite3_exec(g_db, cmd, 0, 0, &err_msg);
    if (res == SQLITE_OK) {
        assert(!err_msg);
        return 1;
    }
    assert(err_msg);
    char *nst_sub = strstr(err_msg, "no such table");
    assert(nst_sub);
    return 0;
}

static int db_get_count_cb(void *out, int len, char **vals, char **cols) {
    if (!*vals) return 0;
    *(size_t *)out = strtoul(*vals, 0, 10);
    return 0;
}

static bool default_task_exists(void) {
    size_t count = 0;
    db_exec_cmd(
        "SELECT COUNT(*) FROM task WHERE DATE(date_created) = "
        "DATE('now') AND name == '" DEFAULT_TASK_NAME "';",
        db_get_count_cb, &count);
    return count;
}

static int get_default_task_id() {
    size_t id = 0;
    db_exec_cmd(
        "SELECT task_id FROM task WHERE DATE(date_created) = "
        "DATE('now') AND name == '" DEFAULT_TASK_NAME "';",
        db_get_count_cb, &id);
    return (int)id;
}

static void ensure_default_tags_exist(void) {
    char cmd[SQL_CMD_CAP] = {0};
    snprintf(cmd, SQL_CMD_CAP, "REPLACE INTO tag (tag_id, name, color) VALUES (0, '%s', %u);", "other",
             g_color[g_cfg.theme][COLOR_SURFACE2]);
    db_exec_cmd(cmd, 0, 0);
}

void db_init(void) {
    int res = sqlite3_open(DB_NAME, &g_db);
    assert(res == SQLITE_OK);

    if (!table_exists("task")) db_exec_cmd(SQLCMD_CREATE_TABLE_TASK, 0, 0);
    if (!is_table_task_valid()) {
        db_exec_cmd("DROP TABLE task", 0, 0);
        db_exec_cmd(SQLCMD_CREATE_TABLE_TASK, 0, 0);
        assert(is_table_task_valid());
    }

    if (!table_exists("tag")) db_exec_cmd(SQLCMD_CREATE_TABLE_TAG, 0, 0);
    if (!is_table_tag_valid()) {
        db_exec_cmd("DROP TABLE tag", 0, 0);
        db_exec_cmd(SQLCMD_CREATE_TABLE_TAG, 0, 0);
        assert(is_table_task_valid());
    }
    ensure_default_tags_exist();

    g_batch_incr_time = batch_incr_time_create();
    g_batch_cmd_str = str_create();

    if (!default_task_exists()) {
        g_default_task_id = db_create_task(DEFAULT_TASK_NAME, 0, 0, 0, 0);
    } else {
        g_default_task_id = get_default_task_id();
    }
}

void db_terminate(void) {
    sqlite3_close(g_db);
    batch_incr_destroy(&g_batch_incr_time);
    str_destroy(&g_batch_cmd_str);
}

void db_print_table(void) { db_exec_cmd(SQLCMD_QUERY_TABLE(task), print_output_cb, 0); }

int db_create_task(const char *name, int done, int left, size_t tag_id, Date_Range *range) {
    Date_Range range0 = {0};
    if (!range) range = &range0;

    static char sql_cmd[SQLCMD_STR_INSERT_CAP] = {0};

    snprintf(sql_cmd, SQLCMD_STR_INSERT_CAP,
             "INSERT INTO task "
             "(name,date_created,done,left,tag_id,date_from,date_to) VALUES "
             "('%s',datetime('now'),%d,%d,%ld,date('%d-%02d-%02d'),"
             "date('%d-%02d-%02d'))",
             name, done, left, tag_id, range->from.year, range->from.month, range->from.day, range->to.year,
             range->to.month, range->to.day);

    db_exec_cmd(sql_cmd, 0, 0);
    int id = sqlite3_last_insert_rowid(g_db);
    return id;
}

void db_set_completed(int id) {
    const char *field = "date_completed";
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = datetime('now') FROM (SELECT "
             "task_id FROM task) WHERE "
             "task.task_id = %d;",
             field, id);
    db_exec_cmd(sql_cmd, 0, 0);
}

void db_incr_done(int id) {
    const char *field = "done";
    char sql_cmd[SQL_CMD_CAP] = {0};
    snprintf(sql_cmd, SQL_CMD_CAP,
             "UPDATE task SET %s = %s + 1 FROM (SELECT "
             "task_id FROM task) WHERE "
             "task.task_id = %d;",
             field, field, id);
    db_exec_cmd(sql_cmd, 0, 0);
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

static Date sql_date_str_to_date(char *str) {
    if (!str) {
        return (Date){0};
    }
    char *year_beg = &str[0];
    char *month_beg = &str[5];
    char *day_beg = &str[8];
    Date result = {
        .year = strtoul(year_beg, 0, 10),
        .month = strtoul(month_beg, 0, 10) - 1,
        .day = strtoul(day_beg, 0, 10),
    };
    return result;
}

static int db_load_todays_task_cb(void *task_ptr_arg, int len, char **vals, char **cols) {
    Task **task_pp = task_ptr_arg;
    Task *task = *task_pp;
    *task = task_create();

    task->db_id = strtoul(vals[LOAD_TASK_VAL_FIELD_ID], 0, 10);

    task->name_len = strlen(vals[LOAD_TASK_VAL_FIELD_NAME]);
    C32 name32[task->name_len + 8];
    ff_utf8_to_utf32(name32, vals[LOAD_TASK_VAL_FIELD_NAME], task->name_len);
    task_set_name(task, name32, task->name_len);

    task->done = strtoul(vals[LOAD_TASK_VAL_FIELD_DONE], 0, 10);
    task->left = strtoul(vals[LOAD_TASK_VAL_FIELD_LEFT], 0, 10);
    task->complete = vals[LOAD_TASK_VAL_FIELD_DATE_COMPLETED];
    task->tag_id = strtoul(vals[LOAD_TASK_VAL_FIELD_TAG_ID], 0, 10);
    task->date_created = sql_date_str_to_date(vals[LOAD_TASK_VAL_FIELD_DATE_CREATED]);
    task->date_range.from = sql_date_str_to_date(vals[LOAD_TASK_VAL_FIELD_DATE_FROM]);
    task->date_range.to = sql_date_str_to_date(vals[LOAD_TASK_VAL_FIELD_DATE_TO]);
    task->diligence = strtof(vals[LOAD_TASK_VAL_FIELD_DILIGENCE],0);
    task->idle= strtof(vals[LOAD_TASK_VAL_FIELD_IDLE],0);
    task->rest= strtof(vals[LOAD_TASK_VAL_FIELD_REST],0);

    *task_pp += 1;
    return 0;
}

void db_get_todays_task(Task *out) {
    db_exec_cmd(
        "SELECT task_id, name, done, left, date_completed, tag_id, date_created, date_from, date_to, diligence, idle, rest FROM task "
        "WHERE "
        "date_created >= "
        "date('now');",
        db_load_todays_task_cb, &out);
}

void db_set_done(int id, int val) {
    const char *field = "done";
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
        "diligence > 0 GROUP BY DATE(date_created)",
        db_get_count_cb, &count);
    return count;
}

static int get_time_activity_cb(void *arg1, int len, char **vals, char **cols) {
    assert(len == 4);
    Time_Activity **tas_pp = arg1;
    Time_Activity *tas = *tas_pp;

    tas->activity.focus = strtof(vals[ACTIVITY_VAL_FIELD_DILIGENCE], 0);
    tas->activity.rest = strtof(vals[ACTIVITY_VAL_FIELD_REST], 0);
    tas->activity.idle = strtof(vals[ACTIVITY_VAL_FIELD_IDLE], 0);
    tas->date = sql_date_str_to_date(vals[ACTIVITY_VAL_FIELD_DATE_CREATED]);
    *tas_pp += 1;
    return 0;
}

void db_get_all_time_activity(Time_Activity *tas) {
    db_exec_cmd(
        "SELECT SUM(diligence) AS diligence, "
        "SUM(rest) AS rest, SUM(idle) AS idle, DATE(date_created) "
        "AS date_created FROM task WHERE diligence > 0 GROUP BY DATE(date_created);",
        get_time_activity_cb, &tas);
}

static int db_get_streak_data_cb(void *arg, int len, char **vals, char **cols) {
    Streak_Data **streak_array_ptr = arg;
    Streak_Data *result = *streak_array_ptr;

    result->start_date = sql_date_str_to_date(vals[STREAK_VAL_FIELD_START_DATE]);
    result->end_date = sql_date_str_to_date(vals[STREAK_VAL_FIELD_END_DATE]);
    result->streak_count = strtoul(vals[STREAK_VAL_FIELD_COUNT], 0, 10);
    *streak_array_ptr += 1;

    return 0;
}

size_t db_get_streak_data_count(void) {
    char *sql_cmd = "SELECT COUNT(*) FROM (" SQLCMD_GET_STREAK_DATA ")";
    size_t result = 0;
    db_exec_cmd(sql_cmd, db_get_count_cb, &result);
    return result;
}

void db_get_streak_data(Streak_Data *out) { db_exec_cmd(SQLCMD_GET_STREAK_DATA, db_get_streak_data_cb, &out); }

int db_get_today_activity_cb(void *arg, int len, char **val, char **col) {
    if (!*val) return 0;
    assert(len == 3);
    Activity *out = arg;
    out->focus = strtof(val[0], 0);
    out->rest = strtof(val[1], 0);
    out->idle = strtof(val[2], 0);
    return 0;
}

Activity db_get_today_activity(void) {
    Activity result = {0};
    db_exec_cmd(
        "SELECT SUM(diligence) AS diligence, "
        "SUM(rest) AS rest, SUM(idle) AS idle "
        "FROM task WHERE DATE(date_created) = DATE('now') "
        "GROUP BY DATE(date_created);",
        db_get_today_activity_cb, &result);
    return result;
}

void db_batch_incr_time(int id, float delta, DB_Incr_Kind kind) {
    Batch_Incr_Time_Item *item = batch_incr_time_find(&g_batch_incr_time, id);
    switch (kind) {
        case INCR_TIME_SPENT_FOCUS: item->focus_delta += delta; break;
        case INCR_TIME_SPENT_REST: item->rest_delta += delta; break;
        case INCR_TIME_SPENT_IDLE: item->idle_delta += delta; break;
    }
}

size_t db_get_streak(void) {
    size_t ret = 0;
    db_exec_cmd(SQLCMD_GET_CURRENT_STREAK, db_get_count_cb, &ret);
    return ret;
}

void db_batch_flush(void) {
    if (!g_batch_incr_time.len) return;
    str_clear(&g_batch_cmd_str);

    for (size_t i = 0; i < g_batch_incr_time.len; i += 1) {
        char cmd[SQL_CMD_CAP] = {0};
        Batch_Incr_Time_Item *item = &g_batch_incr_time.data[i];
        size_t cmd_len = snprintf(cmd, SQL_CMD_CAP,
                                  "UPDATE task SET diligence = diligence+%f, "
                                  "rest = rest+%f, idle = idle+%f  WHERE "
                                  "task.task_id = %d;",
                                  item->focus_delta, item->rest_delta, item->idle_delta, item->id);
        str_cat(&g_batch_cmd_str, cmd, cmd_len);
    }

    str_null_terminate(&g_batch_cmd_str);
    db_exec_cmd(g_batch_cmd_str.data, 0, 0);
    g_batch_incr_time.len = 0;
}

bool db_is_default_task(int id) { return g_default_task_id == id; }

Tag_Id db_create_tag(const char *name, unsigned color) {
    char cmd[SQL_CMD_CAP] = {0};
    size_t cmd_len = snprintf(cmd, SQL_CMD_CAP, "INSERT INTO tag (name, color) VALUES ('%s', %u);", name, color);
    assert(cmd_len);

    db_exec_cmd(cmd, 0, 0);
    int id = sqlite3_last_insert_rowid(g_db);
    return id;
}

void db_delete_tag(int id) {
    char cmd[SQL_CMD_CAP] = {0};
    size_t cmd_len = snprintf(cmd, SQL_CMD_CAP, "DELETE FROM TAG WHERE tag_id=%d;", id);
    assert(cmd_len);

    db_exec_cmd(cmd, 0, 0);
}

size_t db_get_tag_count(void) {
    size_t count = 0;
    db_exec_cmd("SELECT COUNT(*) FROM Tag", db_get_count_cb, &count);
    return count;
}

static int db_get_tags_cb(void *arg1, int len, char **vals, char **cols) {
    Tag **tag_pp = arg1;
    Tag *tag = *tag_pp;

    tag->id = strtoul(vals[TAG_VAL_FIELD_ID], 0, 10);
    tag->color = strtoul(vals[TAG_VAL_FIELD_COLOR], 0, 10);
    size_t name_len = strlen(vals[TAG_VAL_FIELD_NAME]);
    tag->name = calloc(1, name_len + 1);
    memcpy(tag->name, vals[TAG_VAL_FIELD_NAME], name_len);

    *tag_pp += 1;
    return 0;
}

void db_get_tags(Tag *out) { db_exec_cmd("SELECT * FROM Tag", db_get_tags_cb, &out); }
