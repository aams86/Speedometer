#ifndef SPEEDOMETER_MEMORY_H
#define SPEEDOMETER_MEMORY_H

#include "pico/stdlib.h"

//functions for flash storage, need to test and implement 
int lookUpSettings();
int saveSettings(uint32_t current_value);

#endif