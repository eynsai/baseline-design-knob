// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "quantum.h"

uint16_t get_as5600_raw(void);
int16_t get_as5600_delta(void);

#ifndef KNOB_MINIMAL

typedef enum {
    KNOB_MODE_OFF = 0,
#    ifdef ENCODER_ENABLE
KNOB_MODE_ENCODER,
#    endif  // ENCODER_ENABLE
#    ifdef POINTING_DEVICE_ENABLE
KNOB_MODE_WHEEL_VERTICAL,
KNOB_MODE_WHEEL_HORIZONTAL,
KNOB_MODE_DRAG_VERTICAL,
KNOB_MODE_DRAG_HORIZONTAL,
KNOB_MODE_DRAG_DIAGONAL,
#    endif  // POINTING_DEVICE_ENABLE
#    ifdef MIDI_ENABLE
    KNOB_MODE_MIDI,
#    endif  // MIDI_ENABLE
} knob_mode_t;

#    ifdef MIDI_ENABLE
typedef enum {
    MIDI_MODE_SIGNED,
    MIDI_MODE_OFFSET,
    MIDI_MODE_TWOS,
} midi_mode_t;
#    endif  // MIDI_ENABLE

typedef struct {
    knob_mode_t mode;
    float sensitivity;
    bool acceleration;
#    ifdef MIDI_ENABLE
    uint8_t midi_channel;
    uint8_t midi_cc;
    midi_mode_t midi_mode;
#    endif  // MIDI_ENABLE
} knob_config_t;

knob_config_t get_knob_config(void);
void set_knob_config(knob_config_t config);
knob_mode_t get_knob_mode(void);
void set_knob_mode(knob_mode_t mode);
float get_knob_sensitivity(void);
void set_knob_sensitivity(float sensitivity);
bool get_knob_acceleration(void);
void set_knob_acceleration(bool acceleration);

#    ifdef MIDI_ENABLE
uint8_t get_knob_midi_channel(void);
void set_knob_midi_channel(uint8_t channel);
uint8_t get_knob_midi_cc(void);
void set_knob_midi_cc(uint8_t cc);
midi_mode_t get_knob_midi_mode(void);
void set_knob_midi_mode(midi_mode_t mode);
#    endif  // MIDI_ENABLE

#endif  // !KNOB_MINIMAL