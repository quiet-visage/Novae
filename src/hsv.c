#include "hsv.h"

#include <math.h>

#include "config.h"

HSV rgb2hsv(Color col) {
    HSV result = {0};

    float r_normal = col.r / 255.f;
    float g_normal = col.g / 255.f;
    float b_normal = col.b / 255.f;
    float cmax = MAX(MAX(r_normal, g_normal), b_normal);
    float cmin = MIN(MIN(r_normal, g_normal), b_normal);
    float delta = cmax - cmin;
    if (cmax == r_normal) {
        result.hue = 60 * fmodf((g_normal - b_normal) / delta, 6);
    } else if (cmax == b_normal) {
        result.hue = 60 * (((b_normal - r_normal) / delta) + 2);
    } else if (cmax == g_normal) {
        result.hue = 60 * (((r_normal - g_normal) / delta) + 4);
    }

    result.value = cmax;
    result.saturation = delta / cmax;

    return result;
}

Color hsv2rgb(HSV hsv) {
    float chroma = hsv.value * hsv.saturation;
    float hue1 = hsv.hue / 60;
    float x = chroma * (1 - fabsf(fmodf(hue1, 2) - 1));
    float r1 = 0;
    float g1 = 0;
    float b1 = 0;
    if (hue1 >= 0 && hue1 <= 1) {
        r1 = chroma;
        g1 = x;
        b1 = 0;
    } else if (hue1 >= 1 && hue1 <= 2) {
        r1 = x;
        g1 = chroma;
        b1 = 0;
    } else if (hue1 >= 2 && hue1 <= 3) {
        r1 = 0;
        g1 = chroma;
        b1 = x;
    } else if (hue1 >= 3 && hue1 <= 4) {
        r1 = 0;
        g1 = x;
        b1 = chroma;
    } else if (hue1 >= 4 && hue1 <= 5) {
        r1 = x;
        g1 = 0;
        b1 = chroma;
    } else if (hue1 >= 5 && hue1 <= 6) {
        r1 = chroma;
        g1 = 0;
        b1 = x;
    }

    float m = hsv.value - chroma;
    return (Color){(r1 + m) * 255.f, (g1 + m) * 255.f, (b1 + m) * 255.f, 0xff};
}

int hsv2rgb_i(HSV hsv) {
    Color col = hsv2rgb(hsv);
    int result = 0;
    result|=col.r << 24;
    result|=col.g << 16;
    result|=col.b << 8;
    result|=0xff;
    return result;
}
