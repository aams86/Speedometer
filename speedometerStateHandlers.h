#ifndef SPEEDOMETER_STATE_HANDLERS_H
#define SPEEDOMETER_STATE_HANDLERS_H

#include <stdio.h>


//speedometer typedef for different states
typedef enum Speedometer_states
{
  ON,
  CHANGE_UNIT,
  TIMING_1,
  TIMING_2,
  CALCULATE_SPEED,
  DISPLAY_SPEED,
  DISPLAY_SPEED_SENSOR_RESET,
  LOW_POWER,
  WAKE_UP
} Speedometer_states;

void manage_states();
void init_speedometer_alarms();
void init_speedometer_gpio();

#endif