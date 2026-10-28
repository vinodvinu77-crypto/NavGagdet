#pragma once
#include <stdint.h>

typedef struct {
    int kind;        // 0 = turn, 1 = arrived, 2 = idle (no phone), 3 = phone connected, no route yet
    int angle;       // degrees, 0 = straight ahead, +90 = right, -90 = left, +-180 = U-turn
    int dist_m;      // metres to the next manoeuvre
    int speed_kmh;
    int remain_m;    // metres to the destination
    int eta_min;     // minutes to the destination
    char street[48]; // ASCII street name to turn onto
} nav_msg_t;

// All of these must be called with the LVGL lock held.
void ui_init(void);
void ui_apply(const nav_msg_t *m);
void ui_status(const char *title, const char *sub);
