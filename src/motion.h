#pragma once

typedef struct {
    float f;
    float z;
    float r;
    float position[2];
    float velocity[2];
    float previous_input[2];
    float critical_threshold;
} Motion;

Motion motion_new();
void motion_hard_set(Motion* m, float x, float y);
void motion_update(Motion* m, float target[2], float delta_time);
void motion_update_x(Motion* m, float target, float delta_time);
void motion_update_y(Motion* m, float target, float delta_time);
