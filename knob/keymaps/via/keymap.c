// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "knob.h"


// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(MO(1), MO(2), MO(3)),
};
// clang-format on

// ============================================================================
// EEPROM
// ============================================================================

#define KNOB_FLAGS_MASK_MODE            (0b00001111)
#define KNOB_FLAGS_MASK_MIDI_MODE       (0b00110000)
#define KNOB_FLAGS_MASK_ACCELERATION    (0b01000000)
#define KNOB_FLAGS_MASK_REVERSE         (0b10000000)

typedef struct __attribute__((packed)) {
    uint8_t flags;
    uint8_t sensitivity;
    uint16_t encoder_keycode_cw;
    uint16_t encoder_keycode_ccw;
    uint8_t midi_channel;
    uint8_t midi_cc;
} packed_layer_config_t;

static packed_layer_config_t packed_layer_configs[4];

_Static_assert(2 + 4*sizeof(packed_layer_config_t) <= VIA_EEPROM_CUSTOM_CONFIG_SIZE, "VIA custom config size too small");

static inline knob_mode_t get_packed_layer_mode(size_t layer) {
    return (knob_mode_t)(packed_layer_configs[layer].flags & KNOB_FLAGS_MASK_MODE);
}
static inline void set_packed_layer_mode(size_t layer, knob_mode_t mode) {
    uint8_t f = packed_layer_configs[layer].flags;
    f = (uint8_t)((f & (uint8_t)~KNOB_FLAGS_MASK_MODE) | ((uint8_t)mode & KNOB_FLAGS_MASK_MODE));
    packed_layer_configs[layer].flags = f;
}
static inline midi_mode_t get_packed_layer_midi_mode(size_t layer) {
    return (midi_mode_t)((packed_layer_configs[layer].flags & KNOB_FLAGS_MASK_MIDI_MODE) >> 4);
}
static inline void set_packed_layer_midi_mode(size_t layer, midi_mode_t midi_mode) {
    uint8_t f = packed_layer_configs[layer].flags;
    f = (uint8_t)((f & (uint8_t)~KNOB_FLAGS_MASK_MIDI_MODE) | (((uint8_t)midi_mode << 4) & KNOB_FLAGS_MASK_MIDI_MODE));
    packed_layer_configs[layer].flags = f;
}
static inline bool get_packed_layer_acceleration(size_t layer) {
    return (packed_layer_configs[layer].flags & KNOB_FLAGS_MASK_ACCELERATION) != 0;
}
static inline void set_packed_layer_acceleration(size_t layer, bool acceleration) {
    uint8_t f = packed_layer_configs[layer].flags;
    if (acceleration) f |= KNOB_FLAGS_MASK_ACCELERATION; else f &= (uint8_t)~KNOB_FLAGS_MASK_ACCELERATION;
    packed_layer_configs[layer].flags = f;
}
static inline bool get_packed_layer_reverse(size_t layer) {
    return (packed_layer_configs[layer].flags & KNOB_FLAGS_MASK_REVERSE) != 0;
}
static inline void set_packed_layer_reverse(size_t layer, bool reverse) {
    uint8_t f = packed_layer_configs[layer].flags;
    if (reverse) f |= KNOB_FLAGS_MASK_REVERSE; else f &= (uint8_t)~KNOB_FLAGS_MASK_REVERSE;
    packed_layer_configs[layer].flags = f;
}

// ============================================================================
// KNOB LAYER HELPERS
// ============================================================================

uint16_t encoder_keycode_cw = KC_NO;
uint16_t encoder_keycode_ccw = KC_NO;

uint8_t highest_layer = 0;

void unpack_layer_config(void) {
    set_knob_mode(get_packed_layer_mode(highest_layer));
    set_knob_sensitivity(packed_layer_configs[highest_layer].sensitivity);
    set_knob_acceleration(get_packed_layer_acceleration(highest_layer));
    set_knob_reverse(get_packed_layer_reverse(highest_layer));
    set_knob_midi_channel(packed_layer_configs[highest_layer].midi_channel);
    set_knob_midi_cc(packed_layer_configs[highest_layer].midi_cc);
    set_knob_midi_mode(get_packed_layer_midi_mode(highest_layer));
    encoder_keycode_cw = packed_layer_configs[highest_layer].encoder_keycode_cw;
    encoder_keycode_ccw = packed_layer_configs[highest_layer].encoder_keycode_ccw;
}

// ============================================================================
// VIA
// ============================================================================

enum per_layer_value_id_offsets {
    mode_id_offset                  = 0,
    sensitivity_id_offset           = 1,
    reverse_id_offset               = 2,
    acceleration_id_offset          = 3,
    encoder_keycode_cw_id_offset    = 4,
    encoder_keycode_ccw_id_offset   = 5,
    midi_channel_id_offset          = 6,
    midi_cc_id_offset               = 7,
    midi_mode_id_offset             = 8,
};

void custom_config_set_value(uint8_t *data) {
    uint8_t value_id = data[0];
    uint8_t value_id_offset = value_id % 10;
    uint8_t layer = (value_id - value_id_offset) / 10;
    switch (value_id_offset) {
        case mode_id_offset:
            set_packed_layer_mode(layer, data[1]);
            break;
        case sensitivity_id_offset:
            packed_layer_configs[layer].sensitivity = data[1];
            break;
        case reverse_id_offset:
            set_packed_layer_reverse(layer, data[1]);
            break;
        case acceleration_id_offset:
            set_packed_layer_acceleration(layer, data[1]);
            break;
        case encoder_keycode_cw_id_offset:
            packed_layer_configs[layer].encoder_keycode_cw = data[1] << 8 | data[2];
            break;
        case encoder_keycode_ccw_id_offset:
            packed_layer_configs[layer].encoder_keycode_ccw = data[1] << 8 | data[2];
            break;
        case midi_channel_id_offset:
            packed_layer_configs[layer].midi_channel = data[1];
            break;
        case midi_cc_id_offset:
            packed_layer_configs[layer].midi_cc = data[1];
            break;
        case midi_mode_id_offset:
            set_packed_layer_midi_mode(layer, data[1]);
            break;
    }
}

void custom_config_get_value(uint8_t *data) {
    uint8_t value_id = data[0];
    uint8_t value_id_offset = value_id % 10;
    uint8_t layer = (value_id - value_id_offset) / 10;

    switch (value_id_offset) {
        case mode_id_offset:
            data[1] = (uint8_t)get_packed_layer_mode(layer);
            break;
        case sensitivity_id_offset:
            data[1] = packed_layer_configs[layer].sensitivity;
            break;
        case reverse_id_offset:
            data[1] = (uint8_t)get_packed_layer_reverse(layer);
            break;
        case acceleration_id_offset:
            data[1] = (uint8_t)get_packed_layer_acceleration(layer);
            break;
        case encoder_keycode_cw_id_offset:
            data[1] = (uint8_t)((packed_layer_configs[layer].encoder_keycode_cw) >> 8);
            data[2] = (uint8_t)((packed_layer_configs[layer].encoder_keycode_cw) & 0xFF);
            break;
        case encoder_keycode_ccw_id_offset:;
            data[1] = (uint8_t)((packed_layer_configs[layer].encoder_keycode_ccw) >> 8);
            data[2] = (uint8_t)((packed_layer_configs[layer].encoder_keycode_ccw) & 0xFF);
            break;
        case midi_channel_id_offset:
            data[1] = packed_layer_configs[layer].midi_channel;
            break;
        case midi_cc_id_offset:
            data[1] = packed_layer_configs[layer].midi_cc;
            break;
        case midi_mode_id_offset:
            data[1] = (uint8_t)get_packed_layer_midi_mode(layer);
            break;
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);
    if (*channel_id != id_custom_channel) {
        *command_id = id_unhandled;
        return;
    }
    switch (*command_id) {
        case id_custom_set_value:
            custom_config_set_value(value_id_and_data);
            unpack_layer_config();
            break;
        case id_custom_get_value:
            custom_config_get_value(value_id_and_data);
            unpack_layer_config();
            break;
        case id_custom_save:
            via_update_custom_config((const void *)packed_layer_configs, 0, sizeof(packed_layer_configs));  // TODO do we have to implement wear leveling???
            unpack_layer_config();
            break;
        default: {
            *command_id = id_unhandled;
            break;
        }
    }
}

// ============================================================================
// QMK HOOKS
// ============================================================================

// called when EEPROM is reset
void eeconfig_init_user(void) {
    reset_knob_config();
    knob_config_t default_config = get_knob_config();
    for (size_t i = 0; i < 4; i++) {
        set_packed_layer_mode(i, default_config.mode);
        set_packed_layer_midi_mode(i, default_config.midi_mode);
        set_packed_layer_acceleration(i, default_config.acceleration);
        set_packed_layer_reverse(i, default_config.reverse);
        packed_layer_configs[i].sensitivity = default_config.sensitivity;
        packed_layer_configs[i].encoder_keycode_cw = KC_NO;
        packed_layer_configs[i].encoder_keycode_ccw = KC_NO;
        packed_layer_configs[i].midi_channel = default_config.midi_channel;
        packed_layer_configs[i].midi_cc = default_config.midi_cc;
    }
    via_update_custom_config((const void *)packed_layer_configs, 0, sizeof(packed_layer_configs));
}

void keyboard_post_init_user(void) {
    via_read_custom_config((void *)packed_layer_configs, 0, sizeof(packed_layer_configs));
}

layer_state_t layer_state_set_user(layer_state_t state) {
    highest_layer = get_highest_layer(state);
    unpack_layer_config();
    return state;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (clockwise) {
        tap_code(encoder_keycode_cw);
    } else {
        tap_code(encoder_keycode_ccw);
    }
    return false;
}



