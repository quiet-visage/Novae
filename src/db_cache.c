#include "db_cache.h"

#include <assert.h>
#include <string.h>

#include "config.h"
#include "db.h"
#include "today.h"

#define ACTIVITY_MAP_CAP 1024
#define TODAY_SYNC_SEC_PERIOD 0.75f
#define ARRAY_INIT_CAP 256

typedef struct {
    Tag* data;
    size_t len;
    size_t cap;
} Tag_Vec;

Tag_Vec g_tags;

Time_Activity* g_map = 0;
size_t g_map_len = 0;
size_t g_map_cap = ARRAY_INIT_CAP;
size_t g_streak = 0;
float g_day_sync_timer = 0;
float g_max_focus_spent = 0;

static Tag_Vec tag_vec_create() {
    Tag_Vec res = {0};
    res.cap = ARRAY_INIT_CAP;
    res.data = malloc(res.cap);
    return res;
}

static void tag_vec_prealloc(Tag_Vec* m, size_t elem_count) {
    size_t req_len = (m->len + elem_count) * sizeof(*m->data);
    while (req_len > m->cap) {
        m->cap *= 2;
        m->data = realloc(m->data, m->cap);
        assert(m->data);
    }
}

static Tag* tag_vec_find(Tag_Vec* m, int id) {
    for (size_t i = 0; i < m->len; i += 1) {
        if (m->data[i].id == id) return &m->data[i];
    }
    return 0;
}

static void tag_vec_clear(Tag_Vec* m) {
    for (size_t i = 0; i < m->len; i += 1) {
        free(m->data[i].name);
    }
    m->len = 0;
}

static void tag_vec_destroy(Tag_Vec* m) {
    tag_vec_clear(m);
    free(m->data);
}

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
        g_max_focus_spent = MAX(g_max_focus_spent, g_map[i].activity.focus);
    }
}

static void map_sync_with_db() {
    size_t n_records = db_get_all_time_activity_count();
    if (!n_records) return;
    map_prealloc(n_records);
    db_get_all_time_activity(g_map);
    g_map_len = n_records;
    update_max_focus();
    g_streak = db_get_streak();
}

inline float db_cache_get_max_diligence(void) { return g_max_focus_spent; }

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

void db_cache_init(void) {
    g_tags = tag_vec_create();
    map_create();

    map_sync_with_db();
    Date today = today_get_date();

    size_t today_idx = g_map_len ? g_map_len - 1 : g_map_len;
    if (g_map_len && memcmp(&g_map[today_idx].date, &today, sizeof(Date))) {
        map_prealloc(1);
        today_idx += 1;
        g_map_len += 1;
    }
    g_map[today_idx].date = today;
    if (!g_map_len) g_map_len += 1;
}

size_t db_cache_get_streak(void) { return g_streak; }

void db_cache_auto_sync(void) {
    // Periodically sync todays activity
    g_day_sync_timer -= GetFrameTime();
    if (g_day_sync_timer <= 0) {
        Activity todays_activity = db_get_today_activity();
        *map_get_today() = todays_activity;
        g_day_sync_timer = TODAY_SYNC_SEC_PERIOD;
        update_max_focus();
        g_streak = db_get_streak();
    }
}

Activity* db_cache_get_todays_activity(void) { return map_get_today(); }

inline Activity* db_cache_get_activity(Date key) { return map_get(key); }

void db_cache_terminate(void) {
    map_destroy();
    tag_vec_destroy(&g_tags);
}

size_t db_cache_get_activity_count(void) { return g_map_len; }

Time_Activity* db_cache_get_activity_array(void) { return g_map; }

void db_cache_sync_tags(void) {
    size_t count = db_get_tag_count();
    tag_vec_clear(&g_tags);
    tag_vec_prealloc(&g_tags, count);
    db_get_tags(g_tags.data);
    g_tags.len = count;
}

inline Tag* db_cache_get_tag(int id) { return tag_vec_find(&g_tags, id); }

Tag* db_cache_get_default_tag(void) {
    Tag* result = db_cache_get_tag(0);
    assert(result);
    return result;
}

Tag* db_cache_get_tag_array(void) { return g_tags.data; }

size_t db_cache_get_tag_array_len(void) { return g_tags.len; }
