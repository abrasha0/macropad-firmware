#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "config_manager.h"

#define ENCODER_CLK_PIN 14
#define ENCODER_DT_PIN  27
#define ENCODER_SW_PIN  12

void initEncoder();
void processEncoder();

#endif
