// MiniNav for the Waveshare ESP32-C6-Touch-AMOLED-2.16 (480x480, no PSRAM).
// The phone page (docs/app) does the routing and sends the next manoeuvre over Bluetooth LE;
// this firmware only draws it. Lines from the phone:
//   N,<kind>,<angle>,<dist_m>,<speed_kmh>,<remain_m>,<eta_min>,<street>
//   B,<0-100>   brightness        X   back to the idle screen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>

#include "lvgl_bsp.h"
#include "power_bsp.h"
#include "nav_ui.h"
#include "ble_link.h"

static const char *TAG = "mininav";

I2cMasterBus user_i2cbus(7, 8, 0);   // scl, sda, port
static DisplayPort *s_display = NULL;
static QueueHandle_t s_queue;
static volatile int s_brightness = -1;
static volatile bool s_connected = false;

// "N,kind,angle,dist,speed,remain,eta,street"
static bool parse_nav(const char *line, nav_msg_t *m)
{
    int v[6];
    const char *p = line + 2;
    for (int i = 0; i < 6; i++) {
        char *end = NULL;
        v[i] = (int)strtol(p, &end, 10);
        if (end == p || *end != ',') return false;
        p = end + 1;
    }
    memset(m, 0, sizeof(*m));
    m->kind = v[0]; m->angle = v[1]; m->dist_m = v[2];
    m->speed_kmh = v[3]; m->remain_m = v[4]; m->eta_min = v[5];
    strncpy(m->street, p, sizeof(m->street) - 1);
    return true;
}

static void on_line(const char *line)
{
    nav_msg_t m;
    if (line[0] == 'N' && line[1] == ',') {
        if (parse_nav(line, &m)) xQueueOverwrite(s_queue, &m);
    } else if (line[0] == 'B' && line[1] == ',') {
        s_brightness = atoi(line + 2);
    } else if (line[0] == 'X') {
        memset(&m, 0, sizeof(m));
        m.kind = 3;
        xQueueOverwrite(s_queue, &m);
    }
}

static void on_conn(bool connected)
{
    s_connected = connected;
    nav_msg_t m;
    memset(&m, 0, sizeof(m));
    m.kind = connected ? 3 : 2;
    xQueueOverwrite(s_queue, &m);
    ESP_LOGI(TAG, "phone %s", connected ? "connected" : "disconnected");
}

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    Custom_PmicPortInit(&user_i2cbus, 0x34);          // powers the panel (same as Waveshare's demo)
    s_display = new DisplayPort(user_i2cbus, 480, 480);
    Lvgl_PortInit(*s_display);

    s_queue = xQueueCreate(1, sizeof(nav_msg_t));
    if (Lvgl_lock(-1) == ESP_OK) {
        ui_init();
        Lvgl_unlock();
    }
    ble_link_start(on_line, on_conn);

    int64_t last_nav_us = 0;
    bool showing_nav = false;
    while (true) {
        nav_msg_t m;
        if (xQueueReceive(s_queue, &m, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (Lvgl_lock(200) == ESP_OK) {
                ui_apply(&m);
                Lvgl_unlock();
            }
            showing_nav = (m.kind == 0);
            if (showing_nav) last_nav_us = esp_timer_get_time();
        } else if (showing_nav && s_connected && esp_timer_get_time() - last_nav_us > 10LL * 1000000) {
            if (Lvgl_lock(200) == ESP_OK) {
                ui_status("No route data", "Keep the phone page open");
                Lvgl_unlock();
            }
            showing_nav = false;
        }
        int b = s_brightness;
        if (b >= 0) {
            s_brightness = -1;
            if (Lvgl_lock(200) == ESP_OK) {
                s_display->Set_Backlight((uint8_t)(b > 100 ? 100 : b));
                Lvgl_unlock();
            }
        }
    }
}
