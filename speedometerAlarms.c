#include "speedometerAlarms.h"

alarm_callback_t callback;


void set_alarm_callback(alarm_callback_t alarm_callback)
{
    callback = alarm_callback;
}

//enable a single alarm interrupt.  Disable first in case alarm is currently active
void enable_alarm(alarm_type *alarm_type, uint32_t duration) {
    disable_alarm(alarm_type);
    alarm_type -> id = add_alarm_in_ms(duration, callback, NULL, false);
    alarm_type -> flag = 0;
}

//if check if alarm ever existed before disabling
//this is likely unnecessary as the alarm is assigned a new ID when it is set
void disable_alarm(alarm_type *alarm_type) {
    if(alarm_type->id != 0) {
        cancel_alarm(alarm_type->id);
    }
}

