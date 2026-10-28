// Navigation screen for the 480x480 AMOLED: big arrow, distance to the next turn, street, speed, ETA.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "nav_ui.h"

static lv_obj_t *s_dot, *s_street, *s_dist, *s_speed, *s_eta, *s_title, *s_sub;
static lv_obj_t *s_box, *s_path, *s_head;
static lv_point_precise_t s_path_pts[4];
static lv_point_precise_t s_head_pts[3];

static const lv_color_t C_WHITE() { return lv_color_hex(0xFFFFFF); }
static const lv_color_t C_ARROW() { return lv_color_hex(0x38BDF8); }
static const lv_color_t C_DIM()   { return lv_color_hex(0x9CA3AF); }

static lv_obj_t *make_label(const lv_font_t *font, lv_color_t color, lv_align_t align, int y)
{
    lv_obj_t *l = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(l, 440);
    lv_obj_align(l, align, 0, y);
    lv_label_set_text(l, "");
    return l;
}

static void style_line(lv_obj_t *line)
{
    lv_obj_set_style_line_width(line, 24, 0);
    lv_obj_set_style_line_color(line, C_ARROW(), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
}

void ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // connection dot (top-left)
    s_dot = lv_obj_create(scr);
    lv_obj_remove_style_all(s_dot);
    lv_obj_set_size(s_dot, 16, 16);
    lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_dot, lv_color_hex(0x6B7280), 0);
    lv_obj_align(s_dot, LV_ALIGN_TOP_LEFT, 20, 20);

    s_street = make_label(&lv_font_montserrat_28, C_WHITE(), LV_ALIGN_TOP_MID, 28);
    s_dist   = make_label(&lv_font_montserrat_48, C_WHITE(), LV_ALIGN_CENTER, 62);
    s_speed  = make_label(&lv_font_montserrat_28, C_DIM(), LV_ALIGN_BOTTOM_LEFT, -34);
    s_eta    = make_label(&lv_font_montserrat_28, C_DIM(), LV_ALIGN_BOTTOM_RIGHT, -34);
    lv_obj_set_width(s_speed, 220);
    lv_obj_set_width(s_eta, 220);
    lv_obj_set_style_text_align(s_speed, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(s_eta, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(s_speed, LV_ALIGN_BOTTOM_LEFT, 24, -34);
    lv_obj_align(s_eta, LV_ALIGN_BOTTOM_RIGHT, -24, -34);

    // arrow drawing area
    s_box = lv_obj_create(scr);
    lv_obj_remove_style_all(s_box);
    lv_obj_set_size(s_box, 240, 240);
    lv_obj_align(s_box, LV_ALIGN_CENTER, 0, -62);
    s_path = lv_line_create(s_box);
    s_head = lv_line_create(s_box);
    style_line(s_path);
    style_line(s_head);

    // idle / status texts
    s_title = make_label(&lv_font_montserrat_48, C_WHITE(), LV_ALIGN_CENTER, -30);
    s_sub   = make_label(&lv_font_montserrat_28, C_DIM(), LV_ALIGN_CENTER, 50);
    lv_label_set_long_mode(s_sub, LV_LABEL_LONG_MODE_WRAP);

    ui_status("MiniNav", "Waiting for phone...");
}

static void show_nav(bool on)
{
    lv_obj_t *nav[] = { s_box, s_street, s_dist, s_speed, s_eta };
    for (lv_obj_t *o : nav) {
        if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    }
    if (on) {
        lv_obj_add_flag(s_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_sub, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_sub, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_status(const char *title, const char *sub)
{
    show_nav(false);
    lv_label_set_text(s_title, title);
    lv_label_set_text(s_sub, sub);
    lv_obj_set_style_bg_color(s_dot, lv_color_hex(0x6B7280), 0);
}

// Arrow tip at (ex, ey) pointing in direction d (radians, 0 = up, clockwise positive).
static void set_head(float ex, float ey, float d)
{
    float dx = sinf(d), dy = -cosf(d);       // direction of travel
    float px = cosf(d), py = sinf(d);        // perpendicular
    float bx = ex - dx * 44.0f, by = ey - dy * 44.0f;
    s_head_pts[0].x = (lv_value_precise_t)(bx - px * 36.0f); s_head_pts[0].y = (lv_value_precise_t)(by - py * 36.0f);
    s_head_pts[1].x = (lv_value_precise_t)ex;                s_head_pts[1].y = (lv_value_precise_t)ey;
    s_head_pts[2].x = (lv_value_precise_t)(bx + px * 36.0f); s_head_pts[2].y = (lv_value_precise_t)(by + py * 36.0f);
    lv_line_set_points(s_head, s_head_pts, 3);
}

static void set_arrow(int angle)
{
    const float cx = 120.0f;
    if (angle >= 160 || angle <= -160) {                // U-turn
        float s = angle < 0 ? -1.0f : 1.0f;
        s_path_pts[0].x = (lv_value_precise_t)(cx + s * 34); s_path_pts[0].y = 215;
        s_path_pts[1].x = (lv_value_precise_t)(cx + s * 34); s_path_pts[1].y = 80;
        s_path_pts[2].x = (lv_value_precise_t)(cx - s * 34); s_path_pts[2].y = 80;
        s_path_pts[3].x = (lv_value_precise_t)(cx - s * 34); s_path_pts[3].y = 140;
        lv_line_set_points(s_path, s_path_pts, 4);
        set_head(cx - s * 34, 185, (float)M_PI);
        return;
    }
    float a = (float)angle * (float)M_PI / 180.0f;
    float bx = cx, by = 150.0f;                         // bend point
    float ex = bx + sinf(a) * 78.0f, ey = by - cosf(a) * 78.0f;
    s_path_pts[0].x = (lv_value_precise_t)cx; s_path_pts[0].y = 215;
    s_path_pts[1].x = (lv_value_precise_t)bx; s_path_pts[1].y = (lv_value_precise_t)by;
    s_path_pts[2].x = (lv_value_precise_t)ex; s_path_pts[2].y = (lv_value_precise_t)ey;
    lv_line_set_points(s_path, s_path_pts, 3);
    set_head(ex, ey, a);
}

static void fmt_dist(char *out, size_t n, int m)
{
    if (m < 0) m = 0;
    if (m < 100)        snprintf(out, n, "%d m", (m / 5) * 5);
    else if (m < 1000)  snprintf(out, n, "%d m", (m / 10) * 10);
    else if (m < 10000) snprintf(out, n, "%d.%d km", m / 1000, (m % 1000) / 100);
    else                snprintf(out, n, "%d km", m / 1000);
}

void ui_apply(const nav_msg_t *m)
{
    char buf[64];
    switch (m->kind) {
    case 2: ui_status("MiniNav", "Waiting for phone..."); return;
    case 3: ui_status("Connected", "Pick a destination on your phone"); 
            lv_obj_set_style_bg_color(s_dot, lv_color_hex(0x22C55E), 0);
            return;
    case 1: ui_status("Arrived", m->street);
            lv_obj_set_style_bg_color(s_dot, lv_color_hex(0x22C55E), 0);
            return;
    default: break;
    }
    show_nav(true);
    lv_obj_set_style_bg_color(s_dot, lv_color_hex(0x22C55E), 0);
    set_arrow(m->angle);
    fmt_dist(buf, sizeof(buf), m->dist_m);
    lv_label_set_text(s_dist, buf);
    lv_label_set_text(s_street, m->street[0] ? m->street : " ");
    snprintf(buf, sizeof(buf), "%d km/h", m->speed_kmh);
    lv_label_set_text(s_speed, buf);
    char d[24];
    fmt_dist(d, sizeof(d), m->remain_m);
    snprintf(buf, sizeof(buf), "%d min  %s", m->eta_min, d);
    lv_label_set_text(s_eta, buf);
}
