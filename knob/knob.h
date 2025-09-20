// Copyright 2025 Morgan Newell Sun
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "quantum.h"

typedef enum {
    KNOB_MODE_OFF = 0,
    KNOB_MODE_ENCODER,
    KNOB_MODE_WHEEL_VERTICAL,
    KNOB_MODE_WHEEL_HORIZONTAL,
    KNOB_MODE_DRAG_VERTICAL,
    KNOB_MODE_DRAG_HORIZONTAL,
    KNOB_MODE_DRAG_DIAGONAL,
    KNOB_MODE_MIDI,
    // KNOB_MODE_DAVINCI_RESOLVE,
} knob_mode_t;

typedef enum {
    MIDI_MODE_SIGNED,
    MIDI_MODE_OFFSET,
    MIDI_MODE_TWOS,
} midi_mode_t;

uint16_t get_as5600_raw(void);
int16_t get_as5600_delta(void);

knob_mode_t get_knob_mode(void);
void set_knob_mode(knob_mode_t mode);

float get_knob_sensitivity(void);
void set_knob_sensitivity(float sensitivity);

bool get_knob_acceleration(void);
void set_knob_acceleration(bool acceleration);

uint8_t get_knob_midi_channel(void);
void set_knob_midi_channel(uint8_t channel);

uint8_t get_knob_midi_cc(void);
void set_knob_midi_cc(uint8_t cc);

midi_mode_t get_knob_midi_mode(void);
void set_knob_midi_mode(midi_mode_t mode);