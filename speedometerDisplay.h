#ifndef SPEEDOMETER_DISPLAY
#define SPEEDOMETER_DISPLAY

#include "pico/stdlib.h"

#define NUM_UNITS 6

//typedef for units in display, not currently used except for default value
typedef enum {
    UNIT_METERS_PER_SECOND = 0,
    UNIT_MILES_PER_HOUR = 1,
    UNIT_KILOMETERS_PER_HOUR = 2,
    UNIT_INCHES_PER_SECOND = 3,
    UNIT_FEET_PER_SECOND = 4,
    UNIT_CENTIMETERS_PER_SECOND = 5,

} unit_type;

void display_speed(double current_speed, unit_type unit_index);
void update_display(unit_type unit_index, uint32_t value);
void init_display();
void clear_display();
#endif