#include "std_dev.h"

#include <math.h>

#include "config.h"
#include "db_cache.h"
#include "raylib.h"

#define EULERS_NUMBER 2.718281828459045

float normal_probability_density(float x, float std_dev, float mean) {
    float xdifmean = x - mean;
    float variance = std_dev * std_dev;
    float exp = -((xdifmean * xdifmean) / (2 * variance));
    float epow = powf(EULERS_NUMBER, exp);
    float i = 1 / sqrt(M_PI_2 * std_dev);
    return i * epow;
}

int cmp_f(const void* av, const void* bv) {
    float* ap = av;
    float* bp = bv;
    return *ap - *bp;
}

void std_dev_view(void) {
    size_t n = db_cache_get_activity_count();
    Time_Activity* arr = db_cache_get_activity_array();
    float focus_sum = 0;

    float sorted_data[n];
    for (size_t i = 0; i < n; ++i) {
        Activity* data = &arr[i].activity;
        sorted_data[i] = data->focus;
        focus_sum += data->focus;
    }
    float focus_mean = focus_sum / (float)n;
    float sxi = 0;
    for (size_t i = 0; i < n; ++i) {
        Activity* data = &arr[i].activity;
        float xdif = data->focus - focus_mean;
        float xdif2 = xdif * xdif;
        sxi += xdif2;
    }
    float std_dev = sqrtf(sxi / (n - 1));

    qsort(sorted_data, n, 4, cmp_f);

    float x = 200;
    float y = 200;
    float w = 200;
    float h = 200;

    size_t points_cap = n + 2;
    Vector2 points[points_cap];
    Vector2* points_ptr = &points[1];

    float max_p = 0.;
    float min_p = 1.;
    float npd_values[n];
    for (size_t i = 0; i < n; ++i) {
        float data = sorted_data[i];
        npd_values[i] = normal_probability_density(data, std_dev, focus_mean);
        max_p = MAX(max_p, npd_values[i]);
        min_p = MIN(max_p, npd_values[i]);
    }

    for (size_t i = 0; i < n; ++i) {
        float npd = npd_values[i];
        float wp = (float)i / (float)n;
        float p = (npd - min_p) / (max_p - min_p);
        // printf("%f %f %f %f\n", p, max_p, min_p, x);
        float cy = y + (1 - (p * h));
        float cx = x + w * wp;
        points_ptr->x = cx;
        points_ptr->y = cy;
        points_ptr++;
        DrawCircle(cx, cy, 2., WHITE);
    }

    points[0] = points[1];
    points[points_cap - 1] = points[points_cap - 2];

    // for (size_t i = 0; i < (points_cap - (points_cap % 2)); ++i) {
    //     DrawLineBezier(points[i], points[i + 1], 1., WHITE);
    // }
    // DrawSplineBasis(points, n + 2, 1., PINK);
    // printf("%f\n", result);
}
