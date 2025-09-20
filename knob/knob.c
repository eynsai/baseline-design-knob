// Copyright 2025 Morgan Newell Sun (@eynsai)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <math.h>
#include "i2c_master.h"
#include "knob.h"

// ============================================================================
// AS5600
// ============================================================================

#define AS5600_DEV_ADDR 0x6C  // (0x36 << 1)
#define AS5600_REG_ADDR 0x0C
#define AS5600_LENGTH 2
#define AS5600_TIMEOUT 100

uint16_t as5600_raw = 0;
int16_t as5600_delta = 0;

void housekeeping_task_read_as5600(void) {
    // raw
    uint8_t buffer[2];
    i2c_read_register(AS5600_DEV_ADDR, AS5600_REG_ADDR, buffer, AS5600_LENGTH, AS5600_TIMEOUT);
    as5600_raw = ((uint16_t)buffer[0] << 8) | buffer[1];
    
    // delta
    static uint16_t prev_as5600_raw = 0;
    as5600_delta = (int16_t)(as5600_raw - prev_as5600_raw);
    if (as5600_delta > 2048) {
        as5600_delta -= 4096;
    } else if (as5600_delta < -2048) {
        as5600_delta += 4096;
    }
    prev_as5600_raw = as5600_raw;
}

uint16_t get_as5600_raw(void) {
    return as5600_raw;
}

int16_t get_as5600_delta(void) {
    return as5600_delta;
}

// ============================================================================
// RING BUFFERS FOR ACCELERATION
// ============================================================================

#ifndef KNOB_MINIMAL

typedef struct {
    float items[KNOB_ACCELERATION_BUFFER_SIZE];
    float current_sum;
    size_t current_size;
    size_t next_index;
} ring_buffer_t;

static void ring_buffer_reset(ring_buffer_t* rb) {
    rb->current_sum  = 0;
    rb->current_size = 0;
    rb->next_index   = 0;
}

static void ring_buffer_push(ring_buffer_t* rb, float item) {
    if (rb->current_size == KNOB_ACCELERATION_BUFFER_SIZE) {
        rb->current_sum -= rb->items[rb->next_index];
    } else {
        rb->current_size++;
    }
    rb->items[rb->next_index] = item;
    rb->current_sum += item;
    rb->next_index = (rb->next_index + 1) % KNOB_ACCELERATION_BUFFER_SIZE;
}

static float ring_buffer_mean(ring_buffer_t* rb) {
    return rb->current_size > 0 ? rb->current_sum / rb->current_size : 0;
}

// ============================================================================
// MIDI HELPERS
// ============================================================================

#    ifdef MIDI_ENABLE

extern MidiDevice midi_device;

void midi_send_relative_cc(int delta, uint8_t channel, uint8_t cc, midi_mode_t mode) {
    if (delta > 63)  delta = 63;
    if (delta < -63) delta = -63;
    uint8_t value = 0;
    if (delta == 0) {
        return;
        // if (mode == MIDI_MODE_OFFSET) value = 64;
        // else value = 0;
    } else {
        if (delta > 63)  delta = 63;
        if (delta < -63) delta = -63;
        switch (mode) {
            case MIDI_MODE_OFFSET:
                value = (uint8_t)(64 + delta);
                break;
            case MIDI_MODE_TWOS:
                if (delta > 0) value = (uint8_t)delta;
                else {
                    int8_t d = delta;
                    uint8_t v = (uint8_t)d & 0x7F;
                    value = v ? v : 0x40;
                }
                break;
            case MIDI_MODE_SIGNED: {
                uint8_t mag = (uint8_t)((delta >= 0) ? delta : -delta);
                uint8_t sign = (delta < 0) ? 0x40 : 0x00;
                value = (uint8_t)(sign | mag);
                break;
            }
        }
    }
    midi_send_cc(&midi_device, channel, cc, value);
}

#    endif  // MIDI_ENABLE

// ============================================================================
// KNOB FUNCTIONALITY
// ============================================================================

#    define ACCELERATION_CONST_P ((float) KNOB_ACCELERATION_BLEND / (float) KNOB_ACCELERATION_SCALE)
#    define ACCELERATION_CONST_Q ((float) KNOB_ACCELERATION_BLEND + 1.0)
#    define ACCELERATION_CONST_R ((float) KNOB_ACCELERATION_SCALE)

typedef struct {
    uint32_t last_update_time;
    uint32_t last_mouse_time;
    int16_t accumulator;
    float remainder;
    ring_buffer_t acceleration_buffer;
} knob_state_t;

knob_config_t knob_config = {0};
knob_state_t knob_state = {0};

void reset_knob_state(void) {
    knob_state.last_update_time = timer_read32();
    knob_state.accumulator = 0;
    knob_state.remainder = 0;
    ring_buffer_reset(&knob_state.acceleration_buffer);
}

void housekeeping_task_knob_modes(void) {

    // skip everything if the knob is set to off
    if (knob_config.mode == KNOB_MODE_OFF) {
        return;
    }

    // reset state after a period of no activity
    if (as5600_delta == 0) {
        if (timer_elapsed32(knob_state.last_update_time) > KNOB_TIMEOUT_MS) {
            reset_knob_state();
            return;
        }
    }
    knob_state.last_update_time = timer_read32();

    // throttle rate at which actions are performed
    knob_state.accumulator += as5600_delta;
    if (timer_elapsed32(knob_state.last_mouse_time) < KNOB_THROTTLE_MS) {
        return;
    }
    knob_state.last_mouse_time = timer_read32();

    // zero out the accumulator when ready to perform an action
    float delta = knob_state.accumulator;
    knob_state.accumulator = 0;

    // apply acceleration
    if (knob_config.acceleration) {
        float speed = fabsf(delta);
        ring_buffer_push(&knob_state.acceleration_buffer, speed);
        if (delta != 0) {
            // v_out = p * square(min(v_in - r, 0)) + q * (v_in - r) + r
            speed = ring_buffer_mean(&knob_state.acceleration_buffer);
            float speed_offset = speed - ACCELERATION_CONST_R;
            float scale_factor = ACCELERATION_CONST_Q * speed_offset + ACCELERATION_CONST_R;
            if (speed_offset < 0) {
                scale_factor += ACCELERATION_CONST_P * speed_offset * speed_offset;
            }
            scale_factor /= speed;
            delta *= scale_factor;
        }
    }

    // scale delta, interpreting sensitivity differently based on mode
    if (knob_config.mode == KNOB_MODE_OFF) {
        return; // unreachable
#    ifdef ENCODER_ENABLE
    } else if (knob_config.mode == KNOB_MODE_ENCODER) {
        delta *= knob_config.sensitivity * (1.0 / 4096.0);
#    endif  // ENCODER_ENABLE
#    ifdef POINTING_DEVICE_ENABLE
    } else if (knob_config.mode == KNOB_MODE_WHEEL_VERTICAL || knob_config.mode == KNOB_MODE_WHEEL_HORIZONTAL) {
        delta *= knob_config.sensitivity * (120.0 / 4096.0);
    } else if (knob_config.mode == KNOB_MODE_DRAG_VERTICAL || knob_config.mode == KNOB_MODE_DRAG_HORIZONTAL || knob_config.mode == KNOB_MODE_DRAG_DIAGONAL) {
        delta *= knob_config.sensitivity * (30.0 / 4096.0);
#    endif  // POINTING_DEVICE_ENABLE
#    ifdef MIDI_ENABLE
    } else if (knob_config.mode == KNOB_MODE_MIDI) {
        delta *= knob_config.sensitivity * (1.0 / 4096.0);
#    endif  // MIDI_ENABLE
    }

    // truncate to integer and save remainder
    delta += knob_state.remainder;
    int delta_truncated = delta;
    knob_state.remainder = delta - delta_truncated;
    
    // apply action
    if (knob_config.mode == KNOB_MODE_OFF) {
        // unreachable
#    ifdef ENCODER_ENABLE
    } else if (knob_config.mode == KNOB_MODE_ENCODER) {
        while (delta_truncated > 0) {
            encoder_queue_event(0, true);
            delta_truncated -= 1;
        } 
        while (delta_truncated < 0) {
            encoder_queue_event(0, false);
            delta_truncated += 1;
        }
#    endif  // ENCODER_ENABLE
#    ifdef POINTING_DEVICE_ENABLE
    } else if (knob_config.mode == KNOB_MODE_WHEEL_VERTICAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.v += delta_truncated * -1;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_WHEEL_HORIZONTAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.h += delta_truncated;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_DRAG_VERTICAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.y += delta_truncated * -1;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_DRAG_HORIZONTAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.x += delta_truncated;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_DRAG_DIAGONAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.y += delta_truncated * -1;
        mouse.x += delta_truncated;
        pointing_device_set_report(mouse);
#    endif  // POINTING_DEVICE_ENABLE
#    ifdef MIDI_ENABLE
    } else if (knob_config.mode == KNOB_MODE_MIDI) {
        midi_send_relative_cc(delta_truncated, knob_config.midi_channel, knob_config.midi_cc, knob_config.midi_mode);
#    endif  // MIDI_ENABLE
    }
    return;
}

// ============================================================================
// PUBLIC KNOB API
// ============================================================================

knob_mode_t get_knob_mode(void) { return knob_config.mode; }
void set_knob_mode(knob_mode_t mode) {
#    ifdef POINTING_DEVICE_ENABLE
    knob_mode_t prev_mode = knob_config.mode;
    if (prev_mode == KNOB_MODE_DRAG_VERTICAL || prev_mode == KNOB_MODE_DRAG_HORIZONTAL || prev_mode == KNOB_MODE_DRAG_DIAGONAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.buttons = pointing_device_handle_buttons(mouse.buttons, false, POINTING_DEVICE_BUTTON1);
        pointing_device_set_report(mouse);
    }
#    endif  // POINTING_DEVICE_ENABLE
    knob_config.mode = mode;
#    ifdef POINTING_DEVICE_ENABLE
    if (mode == KNOB_MODE_DRAG_VERTICAL || mode == KNOB_MODE_DRAG_HORIZONTAL || mode == KNOB_MODE_DRAG_DIAGONAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.buttons = pointing_device_handle_buttons(mouse.buttons, true, POINTING_DEVICE_BUTTON1);
        pointing_device_set_report(mouse);
    }
#    endif  // POINTING_DEVICE_ENABLE
    reset_knob_state();
}

knob_config_t get_knob_config(void) { return knob_config; }
void set_knob_config(knob_config_t config) { knob_config = config; }
float get_knob_sensitivity(void) { return knob_config.sensitivity; }
void set_knob_sensitivity(float sensitivity) { knob_config.sensitivity = sensitivity; }
bool get_knob_acceleration(void) { return knob_config.acceleration; }
void set_knob_acceleration(bool acceleration) { knob_config.acceleration = acceleration; }

#    ifdef MIDI_ENABLE
uint8_t get_knob_midi_channel(void) { return knob_config.midi_channel; }
void set_knob_midi_channel(uint8_t channel) { knob_config.midi_channel = channel; }
uint8_t get_knob_midi_cc(void) { return knob_config.midi_cc; }
void set_knob_midi_cc(uint8_t cc) { knob_config.midi_cc = cc; }
midi_mode_t get_knob_midi_mode(void) { return knob_config.midi_mode; }
void set_knob_midi_mode(midi_mode_t mode) { knob_config.midi_mode = mode; }
#    endif  // MIDI_ENABLE

#endif // !KNOB_MINIMAL

// ============================================================================
// QMK HOOKS
// ============================================================================

void keyboard_pre_init_kb(void){
    i2c_init();
#ifndef KNOB_MINIMAL
    reset_knob_state();
#endif // !KNOB_MINIMAL
    keyboard_pre_init_user();
}

void housekeeping_task_kb(void) {
    housekeeping_task_read_as5600();
#ifndef KNOB_MINIMAL
    housekeeping_task_knob_modes();
#endif // !KNOB_MINIMAL
    housekeeping_task_user();
}




/*

Stepped functions
DONE - Volume control (+media keys?)
DONE - Arrow keys (up/down, left/right)
DONE - Relative MIDI CC (multiple preset channels)

Smooth functions:
DONE - Smooth scrolling (vertical/horizontal, modifiers)
DONE - Mouse move/drag (vertical/horizontal/diagonal, modifiers)
- DaVinci Resolve

Smooth modifiers:
DONE - sensitivity
DONE - Smoothing window
DONE - Acceleration

Other:
- Layer control and ergonomics
- Indicator lights (vs backlight RGB?)
- MacOS compatibility?
- Switches to enable/disable features

*/