// Copyright 2025 Morgan Newell Sun
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "quantum.h"

typedef enum {
    KNOB_MODE_OFF = 0,
    KNOB_MODE_ENCODER,
    KNOB_MODE_WHEEL_VERTICAL,
    KNOB_MODE_WHEEL_HORIZONTAL,
    KNOB_MODE_POINTER_VERTICAL,
    KNOB_MODE_POINTER_HORIZONTAL,
    KNOB_MODE_POINTER_DIAGONAL,
    KNOB_MODE_MIDI,
    // KNOB_MODE_DAVINCI_RESOLVE,
} knob_mode_t;

typedef enum {
    MIDI_RELATIVE_MODE_OFFSET,   // 64 = no change; 65..127 = +1..+63; 63..1 = -1..-63
    MIDI_RELATIVE_MODE_TWOS,     // 0x00 = no change; 0x01..0x3F = +1..+63; 0x7F..0x40 = -1..-64
    MIDI_RELATIVE_MODE_SIGNED,   // 0x00 = no change; 0x01..0x3F = +1..+63; 0x41..0x7F = -1..-63
} midi_relative_mode_t;

uint16_t get_as5600_raw(void);
int16_t get_as5600_delta(void);

void set_knob_mode(knob_mode_t mode);
knob_mode_t get_knob_mode(void);

void set_knob_acceleration(bool acceleration);
uint16_t get_knob_acceleration(void);

void set_knob_events_per_rotation(float events_per_rotation);
float get_knob_events_per_rotation(void);