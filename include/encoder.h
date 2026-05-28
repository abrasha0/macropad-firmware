#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config_manager.h" // For NUM_LAYERS and currentLayer

// Encoder pins (must match those defined in README.md)
#define ENCODER_CLK_PIN 27   // GPIO27
#define ENCODER_DT_PIN  14   // GPIO14
#define ENCODER_SW_PIN  12   // GPIO12

// Encoder action type (same as keycode in keymap)
typedef uint16_t encoder_action_t;

// Extern declarations for encoder actions per layer (defined in config_manager.cpp)
extern encoder_action_t encoder_left[NUM_LAYERS];
extern encoder_action_t encoder_right[NUM_LAYERS];
extern encoder_action_t encoder_button[NUM_LAYERS];

// Function to initialize the encoder (call from setup)
void initEncoder();

// Function to be called in loop to process encoder events
void processEncoder();

#endif // ENCODER_H