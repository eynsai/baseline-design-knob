// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "baseline_design/knob/knob.h"  // TODO REPLACE WITH #include "knob.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_A, KC_B, KC_C)
};

#if defined(ENCODER_MAP_ENABLE)
    const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
};
#endif
// clang-format on

void keyboard_pre_init_user(void) {

    // TEMPORARY TESTING CODE
    set_knob_mode(KNOB_MODE_ENCODER);
    set_knob_acceleration(false);
    set_knob_sensitivity(10);
    set_knob_midi_channel(0);
    set_knob_midi_cc(0);
    set_knob_midi_mode(MIDI_MODE_SIGNED);

}

// bool process_record_user(uint16_t keycode, keyrecord_t *record) {
//     // if (keycode == KC_A && record->event.pressed) {
//     //     set_knob_sensitivity(get_knob_sensitivity() / 2.0);
//     //     return false;
//     // } else if (keycode == KC_B && record->event.pressed) {
//     //     if (get_knob_acceleration()) {
//     //         set_knob_acceleration(false);
//     //     } else {
//     //         set_knob_acceleration(true);
//     //     }
//     //     return false;
//     // } else if (keycode == KC_C && record->event.pressed) {
//     //     set_knob_sensitivity(get_knob_sensitivity() * 2.0);
//     //     return false;
//     // }
//     if (keycode == KC_A) {
//         if (record->event.pressed) {
//             set_knob_mode(KNOB_MODE_DRAG_DIAGONAL);
//         } else {
//             set_knob_mode(KNOB_MODE_WHEEL_VERTICAL);
//         }
//         return false;
//     } 
//     return true;
// }
