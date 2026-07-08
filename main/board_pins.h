#pragma once

#include "driver/gpio.h"

/*
 * Board pin map generated from pcb/smart-doorbell/smart-doorbell.net.
 * The ESP32-S3 GPIO matrix makes many peripherals remappable, but after PCB
 * fabrication these constants must match the routed nets.
 */

// Wake/button and indicators
#define DOORBELL_BUTTON_PIN GPIO_NUM_2   // /DOORBELL_IN
#define BATTERY_ADC_PIN     GPIO_NUM_1   // /GPIO1, 100k/100k divider
#define PIR_OUT_PIN         GPIO_NUM_42  // /PIR_OUT optional header
#define STATUS_LED_PIN      GPIO_NUM_47  // /STATUS_LED
#define BUTTON_LED_PIN      GPIO_NUM_48  // /BTN_LED

// MAX98357A amplifier control
#define AMP_EN_PIN          GPIO_NUM_44  // /AMP_EN -> MAX98357A SD_MODE

// OV5640 camera on 24-pin FPC J3
#define CAM_PIN_D0          18           // /CAM_D0
#define CAM_PIN_D1          16           // /CAM_D1
#define CAM_PIN_D2          15           // /CAM_D2
#define CAM_PIN_D3          17           // /CAM_D3
#define CAM_PIN_D4          8            // /CAM_D4
#define CAM_PIN_D5          10           // /CAM_D5
#define CAM_PIN_D6          11           // /CAM_D6
#define CAM_PIN_D7          13           // /CAM_D7
#define CAM_PIN_PCLK        9            // /CAM_PCLK
#define CAM_PIN_XCLK        12           // /CAM_XCLK
#define CAM_PIN_HREF        14           // /CAM_HREF
#define CAM_PIN_PWDN        21           // /CAM_PWDN
#define CAM_PIN_SIOD        38           // /CAM_SDA
#define CAM_PIN_SIOC        39           // /CAM_SCL
#define CAM_PIN_RESET       40           // /CAM_RST
#define CAM_PIN_VSYNC       41           // /CAM_VSYNC

// I2S audio
#define I2S_AUDIO_WS        GPIO_NUM_4   // /GPIO4 -> ICS WS, MAX LRCLK
#define I2S_AUDIO_SCK       GPIO_NUM_5   // /GPIO5 -> ICS SCK, MAX BCLK
#define I2S_MIC_SD          GPIO_NUM_6   // /GPIO6 -> ICS SD
#define I2S_SPK_SD          GPIO_NUM_7   // /GPIO7 -> MAX DIN

// Native USB. Reserved for USB only.
#define USB_D_MINUS_PIN     GPIO_NUM_19
#define USB_D_PLUS_PIN      GPIO_NUM_20
