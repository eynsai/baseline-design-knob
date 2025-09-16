#pragma once

// core knob functionality
#define I2C_MASTER
#define I2C1_SDA_PIN GP2
#define I2C1_SCL_PIN GP3
#define I2C_MASTER_ENABLE
#define POINTING_DEVICE_HIRES_SCROLL_ENABLE
#define MOUSE_EXTENDED_REPORT
#define WHEEL_EXTENDED_REPORT
#define ENCODER_A_PINS { F4 }
#define ENCODER_B_PINS { F5 }

// rgb
#define WS2812_DI_PIN B6
#define RGBLIGHT_LED_COUNT 3
#define RGBLIGHT_SLEEP

// misc qmk features
#define BOOTMAGIC_ROW 0
#define BOOTMAGIC_COLUMN 0
