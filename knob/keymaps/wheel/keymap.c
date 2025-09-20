// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "knob.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_1, KC_2, KC_3)
};
// clang-format on

void keyboard_pre_init_user(void) {
    set_knob_mode(KNOB_MODE_WHEEL_VERTICAL);
    set_knob_acceleration(true);
    set_knob_sensitivity(8.0);
}

void housekeeping_task_user(void) {
    static float hue = 115.0;
    static uint32_t last_update_time = 0;
    if (timer_elapsed32(last_update_time) < 1) {
        return;
    }
    last_update_time  = timer_read32();
    hue += (((float) get_as5600_delta()) / 128.0);
    while (hue >= 256) {
        hue -= 256;
    }
    while (hue < 0) {
        hue += 256;
    }
    rgblight_sethsv_noeeprom((uint8_t) hue, 255, RGBLIGHT_LIMIT_VAL);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    static bool fast = false;

    // left button toggles between horizontal and vertical
    if (keycode == KC_1) {
        if (!record->event.pressed) return false;
        if (get_knob_mode() == KNOB_MODE_WHEEL_VERTICAL) {
            set_knob_mode(KNOB_MODE_WHEEL_HORIZONTAL);
        } else {
            set_knob_mode(KNOB_MODE_WHEEL_VERTICAL);
        }
        return false;
    }

    // middle button toggles acceleration on/off
    if (keycode == KC_2) {
        if (!record->event.pressed) return false;
        if (get_knob_acceleration()) {
            set_knob_acceleration(false);
        } else {
            set_knob_acceleration(true);
        }
        return false;
    }

    // right button toggles between two speeds
    if (keycode == KC_3) {
        if (!record->event.pressed) return false;
        if (fast) {
            set_knob_sensitivity(8.0);
            fast = false;
        } else {
            set_knob_sensitivity(24.0);
            fast = true;
        }
        return false;
    }

    return true;
}
