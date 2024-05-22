#include "chrono.h"

#include <raylib.h>

double min_to_sec(double min) { return min * 60; }

double chrono_mins(Chrono *m) { return m->secs_left / 60; }

double chrono_secs(Chrono *m) { return m->secs_left; }

U8 chrono_clock_mins(Chrono *m) { return ((U8)chrono_mins(m)) % 60; }

U8 chrono_clock_secs(Chrono *m) { return ((U8)m->secs_left) % 60; }

Chrono_Stat chrono_update(Chrono *m) {
    if (m->secs_left <= 0) return CHRONO_DONE;

    double delta = GetFrameTime();
    m->secs_left -= delta;

    if (m->secs_left <= 0) {
        m->secs_left = 0;
        return CHRONO_DONE;
    }
    return CHRONO_RUNNING;
}
