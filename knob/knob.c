// Copyright 2025 Morgan Newell Sun
// SPDX-License-Identifier: GPL-2.0-or-later

#include "knob.h"

#include "quantum.h"
#include "encoder.h"
#include "pointing_device.h"
#include "i2c_master.h"

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
    float items[MOUSE_SMOOTHING_BUFFER_SIZE];
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
    if (rb->current_size == MOUSE_SMOOTHING_BUFFER_SIZE) {
        rb->current_sum -= rb->items[rb->next_index];
    } else {
        rb->current_size++;
    }
    rb->items[rb->next_index] = item;
    rb->current_sum += item;
    rb->next_index = (rb->next_index + 1) % MOUSE_SMOOTHING_BUFFER_SIZE;
}

static float ring_buffer_mean(ring_buffer_t* rb) {
    return rb->current_size > 0 ? rb->current_sum / rb->current_size : 0;
}

// ============================================================================
// KNOB FUNCTIONALITY
// ============================================================================

typedef struct {

    // shared
    knob_mode_t mode;

    // step counting
    int16_t step_size;

    // mouse
    float x_multiplier;
    float y_multiplier;
    float h_multiplier;
    float v_multiplier;

} knob_config_t;
knob_config_t knob_config = {0};

typedef struct {

    // shared
    uint32_t last_update_time;

    // step counting
    int16_t partial_step_accumulator;
    int16_t whole_step_accumulator;

    // mouse
    uint32_t last_mouse_time;
    int16_t mouse_accumulator;
    ring_buffer_t mouse_smoothing_buffer;

} knob_state_t;
knob_state_t knob_state = {0};

void accumulate_steps(void) {
    knob_state.partial_step_accumulator += as5600_delta;
    while (knob_state.partial_step_accumulator >= knob_config.step_size) {
        knob_state.partial_step_accumulator -= knob_config.step_size;
        knob_state.whole_step_accumulator += 1;
    }
    while (knob_state.partial_step_accumulator <= -knob_config.step_size) {
        knob_state.partial_step_accumulator += knob_config.step_size;
        knob_state.whole_step_accumulator -= 1;
    }
}

void reset_knob_state(void) {
    knob_state.last_update_time = timer_read32();
    knob_state.partial_step_accumulator = 0;
    knob_state.whole_step_accumulator = 0;
    knob_state.mouse_accumulator = 0;
    ring_buffer_reset(&knob_state.mouse_smoothing_buffer);
}

void housekeeping_task_knob(void) {

    // reset state after a period of no activity
    if (as5600_delta == 0) {
        if (timer_elapsed32(knob_state.last_update_time) > KNOB_TIMEOUT_MS) {
            reset_knob_state();
        }
        return;
    }
    knob_state.last_update_time = timer_read32();

    // disable the knob
    if (knob_config.mode == KNOB_MODE_OFF) {
        return;

    // send keypresses corresponding to the encoder map
    } else if (knob_config.mode == KNOB_MODE_ENCODER) {
        accumulate_steps();
        while (knob_state.whole_step_accumulator > 0) {
            encoder_queue_event(0, true); // send a CW encoder event
            knob_state.whole_step_accumulator -= 1;
        } 
        while (knob_state.whole_step_accumulator < 0) {
            encoder_queue_event(0, false); // send a CCW encoder event
            knob_state.whole_step_accumulator += 1;
        }
        return;

    // move the wheel (for scrolling) or the pointer (for click-and-drag)
    } else if (knob_config.mode >= KNOB_MODE_WHEEL_VERTICAL && knob_config.mode <= KNOB_MODE_POINTER_DIAGONAL) {
        knob_state.mouse_accumulator += as5600_delta;
        if (timer_elapsed32(knob_state.last_mouse_time) < MOUSE_THROTTLE_MS) {
            return;
        }
        knob_state.last_mouse_time = timer_read32();
        ring_buffer_push(&knob_state.mouse_smoothing_buffer, knob_state.mouse_accumulator);
        float delta = ring_buffer_mean(&knob_state.mouse_smoothing_buffer);
        knob_state.mouse_accumulator = 0;
        // TODO: add acceleration?
        report_mouse_t mouse = pointing_device_get_report();
        if (knob_config.mode == KNOB_MODE_WHEEL_VERTICAL) {
            mouse.v = (mouse_hv_report_t) delta * knob_config.v_multiplier;
        } else if (knob_config.mode == KNOB_MODE_WHEEL_HORIZONTAL) {
            mouse.h = (mouse_hv_report_t )delta * knob_config.h_multiplier;
        } else if (knob_config.mode == KNOB_MODE_POINTER_VERTICAL) {
            mouse.y = (mouse_xy_report_t) delta * knob_config.y_multiplier;
        } else if (knob_config.mode == KNOB_MODE_POINTER_HORIZONTAL) {
            mouse.x = (mouse_xy_report_t) delta * knob_config.x_multiplier;
        } else {  // if (knob_config.mode == KNOB_MODE_POINTER_DIAGONAL) {
            mouse.x = (mouse_xy_report_t) delta * knob_config.x_multiplier;
            mouse.y = (mouse_xy_report_t) delta * knob_config.y_multiplier;
        }
        pointing_device_set_report(mouse);
        return;
    }


}

void set_knob_mode(knob_mode_t mode) {
    reset_knob_state();
    knob_config.mode = mode;
}

knob_mode_t get_knob_mode(void) {
    return knob_config.mode;
}

// ============================================================================
// QMK HOOKS
// ============================================================================

void keyboard_pre_init_kb(void){
    i2c_init();
    reset_knob_state();
    keyboard_pre_init_user();  // weak function allows user to implement custom functionality

    // TEMPORARY TESTING CODE
    knob_config.mode = KNOB_MODE_ENCODER;
    knob_config.step_size = 300;
    knob_config.h_multiplier = 8;
    knob_config.v_multiplier = -8;
    knob_config.x_multiplier = 1;
    knob_config.y_multiplier = -1;
    set_knob_mode(KNOB_MODE_ENCODER);
}

void housekeeping_task_kb(void) {
    housekeeping_task_read_as5600();
    housekeeping_task_knob();
    housekeeping_task_user();  // weak function allows user to implement custom functionality
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
DONE - Sensitivity
DONE - Smoothing window
- Acceleration

Other:
- Layer control and ergonomics
- Indicator lights (vs backlight RGB?)
- MacOS compatibility?

*/