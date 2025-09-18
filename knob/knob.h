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
    // KNOB_MODE_MIDI,
    // KNOB_MODE_DAVINCI_RESOLVE,
} knob_mode_t;

uint16_t get_as5600_raw(void);
int16_t get_as5600_delta(void);

void set_knob_mode(knob_mode_t mode);
knob_mode_t get_knob_mode(void);

void set_knob_step_size(uint16_t step_size);
uint16_t get_knob_step_size(void);

void set_knob_acceleration(bool acceleration);
uint16_t get_knob_acceleration(void);

void set_knob_sensitivity(float sensitivity);
float get_knob_sensitivity(void);