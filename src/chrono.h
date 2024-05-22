#pragma once

typedef unsigned short U8;

typedef enum {
    CHRONO_RUNNING,
    CHRONO_DONE,
} Chrono_Stat;

typedef struct {
    double secs_left;
} Chrono;

double min_to_sec(double min);
double sec_to_ms(double min);
double chrono_mins(Chrono* m);
double chrono_secs(Chrono* m);
U8 chrono_clock_mins(Chrono* m);
U8 chrono_clock_secs(Chrono* m);
Chrono_Stat chrono_update(Chrono* m);
