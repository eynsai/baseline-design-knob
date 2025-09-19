// Copyright 2025 Morgan Newell Sun
// SPDX-License-Identifier: GPL-2.0-or-later

#include <math.h>

#include "quantum.h"
#include "encoder.h"
#include "pointing_device.h"
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
// RING BUFFERS
// ============================================================================

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
// KNOB FUNCTIONALITY
// ============================================================================

#define ACCELERATION_CONST_P ((float) KNOB_ACCELERATION_BLEND / (float) KNOB_ACCELERATION_SCALE)
#define ACCELERATION_CONST_Q ((float) KNOB_ACCELERATION_BLEND + 1.0)
#define ACCELERATION_CONST_R ((float) KNOB_ACCELERATION_SCALE)

typedef struct {
    knob_mode_t mode;
    bool acceleration;
    float events_per_rotation;
} knob_config_t;
knob_config_t knob_config = {0};

typedef struct {
    uint32_t last_update_time;
    uint32_t last_mouse_time;
    int16_t accumulator;
    float remainder;
    ring_buffer_t acceleration_buffer;

} knob_state_t;
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

    // scale delta, interpreting events_per_rotation differently based on mode
    switch (knob_config.mode) {
        case KNOB_MODE_OFF:
            return;  // unreachable
        case KNOB_MODE_ENCODER:
            delta *= knob_config.events_per_rotation * (1.0 / 4096.0);
            break;
        case KNOB_MODE_WHEEL_VERTICAL:
        case KNOB_MODE_WHEEL_HORIZONTAL:
            delta *= knob_config.events_per_rotation * (120.0 / 4096.0);
            break;
        case KNOB_MODE_POINTER_VERTICAL:
        case KNOB_MODE_POINTER_HORIZONTAL:
        case KNOB_MODE_POINTER_DIAGONAL:
            delta *= knob_config.events_per_rotation * (300.0 / 4096.0);
            break;
    }

    // truncate to integer and save remainder
    delta += knob_state.remainder;
    int delta_truncated = delta;
    knob_state.remainder = delta - delta_truncated;
    
    // apply action
    if (knob_config.mode == KNOB_MODE_ENCODER) {
        while (delta_truncated > 0) {
            encoder_queue_event(0, true);
            delta_truncated -= 1;
        } 
        while (delta_truncated < 0) {
            encoder_queue_event(0, false);
            delta_truncated += 1;
        }
    } else if (knob_config.mode == KNOB_MODE_WHEEL_VERTICAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.v += delta_truncated * -1;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_WHEEL_HORIZONTAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.h += delta_truncated;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_POINTER_VERTICAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.y += delta_truncated * -1;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_POINTER_HORIZONTAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.x += delta_truncated;
        pointing_device_set_report(mouse);
    } else if (knob_config.mode == KNOB_MODE_POINTER_DIAGONAL) {
        report_mouse_t mouse = pointing_device_get_report();
        mouse.y += delta_truncated * -1;
        mouse.x += delta_truncated;
        pointing_device_set_report(mouse);
    }
    return;
}

// ============================================================================
// PUBLIC KNOB API
// ============================================================================

void set_knob_mode(knob_mode_t mode) { reset_knob_state(); knob_config.mode = mode; }
knob_mode_t get_knob_mode(void) { return knob_config.mode; }

void set_knob_acceleration(bool acceleration) { knob_config.acceleration = acceleration; }
uint16_t get_knob_acceleration(void) { return knob_config.acceleration; }

void set_knob_events_per_rotation(float events_per_rotation) { knob_config.events_per_rotation = events_per_rotation; }
float get_knob_events_per_rotation(void) { return knob_config.events_per_rotation; }

// ============================================================================
// QMK HOOKS
// ============================================================================

void keyboard_pre_init_kb(void){
    i2c_init();
    reset_knob_state();
    keyboard_pre_init_user();
}

void housekeeping_task_kb(void) {
    housekeeping_task_read_as5600();
    housekeeping_task_knob_modes();
    housekeeping_task_user();
}




/*

Stepped functions
DONE - Volume control (+media keys?)
DONE - Arrow keys (up/down, left/right)
- Relative MIDI CC (multiple preset channels)

Smooth functions:
DONE - Smooth scrolling (vertical/horizontal, modifiers)
DONE - Mouse move/drag (vertical/horizontal/diagonal, modifiers)
- DaVinci Resolve

Smooth modifiers:
DONE - events_per_rotation
DONE - Smoothing window
DONE - Acceleration

Other:
- Layer control and ergonomics
- Indicator lights (vs backlight RGB?)
- MacOS compatibility?

*/