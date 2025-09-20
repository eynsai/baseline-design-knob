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

void colormap(int index, bool on) {
    uint8_t val = on ? RGBLIGHT_LIMIT_VAL : (RGBLIGHT_LIMIT_VAL >> 2);
    switch (index) {
        case 0:
            rgblight_sethsv_noeeprom(0, 255, val);
            return;
        case 1:
            rgblight_sethsv_noeeprom(12, 255, val);
            return;
        case 2:
            rgblight_sethsv_noeeprom(35, 255, val);
            return;
        case 3:
            rgblight_sethsv_noeeprom(80, 255, val);
            return;
        case 4:
            rgblight_sethsv_noeeprom(105, 255, val);
            return;
        case 5:
            rgblight_sethsv_noeeprom(165, 255, val);
            return;
        case 6:
            rgblight_sethsv_noeeprom(185, 255, val);
            return;
        case 7:
            rgblight_sethsv_noeeprom(215, 255, val);
            return;
        default:
            return;
    }
}

void keyboard_pre_init_user(void) {

    // set knob settings
    set_knob_mode(KNOB_MODE_MIDI);
    set_knob_sensitivity(128);
    set_knob_midi_channel(0);  // not necessary since it's the default
    set_knob_midi_cc(0);  // not necessary since it's the default

    // set the relative midi mode (consult your daw manual)
    set_knob_midi_mode(MIDI_MODE_SIGNED);
    // set_knob_midi_mode(MIDI_MODE_OFFSET);
    // set_knob_midi_mode(MIDI_MODE_TWOS);

    // initialize rgb
    colormap(0, true);

}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    static int cc = 0;

    // middle button toggles midi on/off
    if (keycode == KC_2) {
        if (!record->event.pressed) return false;
        if (get_knob_mode() == KNOB_MODE_MIDI) {
            set_knob_mode(KNOB_MODE_OFF);
            colormap(cc, false);
        } else {
            set_knob_mode(KNOB_MODE_MIDI);
            colormap(cc, true);
        }
        return false;
    }

    // left/right buttons cycle through cc values
    if (keycode == KC_1 || keycode == KC_3) {
        if (!record->event.pressed) return false;
        cc = cc + (keycode == KC_3 ? 1 : -1);
        if (cc == 8) {
            cc = 0;
        } else if (cc == -1) {
            cc = 7;
        }
        set_knob_midi_cc(cc);
        colormap(cc, get_knob_mode() == KNOB_MODE_MIDI);
        return false;
    }

    return true;
}
