#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ble_link_line_cb)(const char *line);   // one complete text line from the phone
typedef void (*ble_link_conn_cb)(bool connected);

// Starts NimBLE, advertises as "MiniNav" and delivers newline-terminated text lines.
// NVS must already be initialised.
void ble_link_start(ble_link_line_cb on_line, ble_link_conn_cb on_conn);

#ifdef __cplusplus
}
#endif
