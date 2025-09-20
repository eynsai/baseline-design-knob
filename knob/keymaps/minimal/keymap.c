// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "knob.h"
#include QMK_KEYBOARD_H

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_A, KC_B, KC_C)
};
// clang-format on

void housekeeping_task_user(void) {
    report_mouse_t mouse = pointing_device_get_report();
    mouse.v += get_as5600_delta() * -1;
    pointing_device_set_report(mouse);
}
