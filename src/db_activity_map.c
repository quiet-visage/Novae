#include "db_activity_map.h"

#include <assert.h>
#include <string.h>

#include "db.h"
#include "today.h"

#define ACTIVITY_MAP_CAP 1024
#define TODAY_SYNC_SEC_PERIOD 4
#define ARRAY_INIT_CAP 256

Time_Activity* g_map = 0;
size_t g_map_len = 0;
size_t g_map_cap = ARRAY_INIT_CAP;
float g_day_sync_timer = 0;
float g_max_focus_spent = 0;

static void map_prealloc(size_t n_elements) {
    size_t req_cap = (g_map_len + n_elements) * sizeof(*g_map);
    while (req_cap > g_map_cap) {
        g_map_cap *= 2;
        g_map = realloc(g_map, g_map_cap);
        assert(g_map);
    }
}

static void update_max_focus(void) {
    g_max_focus_spent = 0;
    for (size_t i = 0; i < g_map_len; i += 1) {
        g_max_focus_spent += g_map[i].activity.focus;
    }
}

static void map_sync_with_db() {
    size_t n_records = db_get_all_time_activity_count();
    if (!n_records) return;
    map_prealloc(n_records);
    db_get_all_time_activity(g_map);
    g_map_len = n_records;
    update_max_focus();
}

float db_activity_get_max_focus(void) { return g_max_focus_spent; }

inline float db_activity_get_max_focus_spent(void) {
    return g_max_focus_spent;
}

static Activity* map_get(Date date) {
    for (size_t i = 0; i < g_map_len; i += 1) {
        bool found = !memcmp(&g_map[i].date, &date, sizeof(date));
        if (!found) continue;
        return &g_map[i].activity;
    }
    return 0;
}

static Activity* map_get_today(void) {
    size_t idx = g_map_len ? g_map_len - 1 : 0;
    return &g_map[idx].activity;
}

static void map_create(void) {
    g_map_cap = ARRAY_INIT_CAP;
    g_map = malloc(ARRAY_INIT_CAP);
    assert(g_map);
    g_map_len = 0;
}

static void map_destroy(void) { free(g_map); }

void db_activity_init(void) {
    map_create();

    map_sync_with_db();
    Date today = today_get_date();

    size_t today_idx = g_map_len ? g_map_len - 1 : g_map_len;
    if (g_map_len &&
        memcmp(&g_map[today_idx].date, &today, sizeof(Date))) {
        map_prealloc(1);
        today_idx += 1;
        g_map_len += 1;
    }
    g_map[today_idx].date = today;
    if (!g_map_len) g_map_len += 1;
}

void db_activity_auto_sync(void) {
    // Periodically sync todays activity
    g_day_sync_timer -= GetFrameTime();
    if (g_day_sync_timer <= 0) {
        Activity todays_activity = db_get_today_activity();
        *map_get_today() = todays_activity;
        g_day_sync_timer = TODAY_SYNC_SEC_PERIOD;
        update_max_focus();
    }
}

inline Activity* db_activity_get(Date key) { return map_get(key); }

void db_acitivity_terminate(void) { map_destroy(); }

size_t db_activity_count(void) { return g_map_len; }

Time_Activity* db_activity_get_array(void) { return g_map; }
