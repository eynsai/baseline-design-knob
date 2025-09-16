#include "baseline_design/knob/knob.h"
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

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if ((keycode == KC_A || keycode == KC_B || keycode == KC_C) && (!record->event.pressed)) {
        set_knob_mode(KNOB_MODE_ENCODER);
        return false;
    }
    if (keycode == KC_A) {
        set_knob_mode(KNOB_MODE_WHEEL_VERTICAL);
        return false;
    } else if (keycode == KC_B) {
        set_knob_mode(KNOB_MODE_WHEEL_HORIZONTAL);
        return false;
    } else if (keycode == KC_C) {
        set_knob_mode(KNOB_MODE_POINTER_DIAGONAL);
        return false;
    }
    return true;
}
