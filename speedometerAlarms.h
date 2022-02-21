#ifndef SPEEDOMETER_ALARMS_H
#define SPEEDOMETER_ALARMS_H
#include "hardware/timer.h"
#include "pico/stdlib.h"

//alarm_type struct to contain the alarm ID and the alarm flag
typedef struct alarm_type {
    alarm_id_t id;
    uint8_t flag;
} alarm_type;

//function to initialize alarm callback
void set_alarm_callback(alarm_callback_t alarm_callback);

//function to enable an alarm, sets the alarm ID of the alarm_type struct
void enable_alarm(alarm_type *alarm_type, uint32_t duration);

//function to disable an active alarm, will check that alarm exsists before disabling
void disable_alarm(alarm_type *alarm_type);

#endif