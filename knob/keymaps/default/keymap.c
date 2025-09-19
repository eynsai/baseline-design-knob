#include "baseline_design/knob/knob.h"
#include "keyboard.h"
#include "quantum.h"
#include QMK_KEYBOARD_H

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
    set_knob_mode(KNOB_MODE_POINTER_DIAGONAL);
    set_knob_acceleration(false);
    set_knob_events_per_rotation(10);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == KC_A && record->event.pressed) {
        set_knob_events_per_rotation(get_knob_events_per_rotation() / 2.0);
        return false;
    } else if (keycode == KC_B && record->event.pressed) {
        if (get_knob_acceleration()) {
            set_knob_acceleration(false);
        } else {
            set_knob_acceleration(true);
        }
        return false;
    } else if (keycode == KC_C && record->event.pressed) {
        set_knob_events_per_rotation(get_knob_events_per_rotation() * 2.0);
        return false;
    }
    return true;
}
