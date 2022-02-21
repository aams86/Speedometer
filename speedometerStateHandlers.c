#include <stdio.h>
#include <hardware/sync.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "speedometerStateHandlers.h"
#include "speedometerAlarms.h"
#include "speedometerDisplay.h"
#include "speedometerMemory.h"


//value to represent physical distance between sensors
#define DISTANCE_MM 32
#define VELOCITY_CONSTANT (DISTANCE_MM * 1000) // conversions from mm->m and us to s contained here

//these values represent the timer quantities for each of the alarm timers
#define SENSOR_TIMEOUT 2000
#define SENSOR_RESET_TIME 200
#define DISPLAY_TIMEOUT 15000
#define UNIT_CHANGE_TIMEOUT 10000
#define BUTTON_PRESS_TIMEOUT 100

//The gpio pins for the sensors and LEDs
#define LED1_PIN 21
#define LED2_PIN 28
#define SENSOR1_PIN 27
#define SENSOR2_PIN 26
#define BUTTON_PIN 22


//globals for managing current values and states
double current_speed = 0;
unit_type unit_index = UNIT_METERS_PER_SECOND;
int counter1 = 0;
Speedometer_states speedometer_state = ON;
Speedometer_states previous_speedometer_state = ON;

//flags for gpio interrupts
volatile uint8_t button_flag = 0;
volatile uint8_t sensor_1_flag = 0;
volatile uint8_t sensor_2_flag = 0;

//alarm_type structs for alarm interrupts, initialize id and flag state to zero
alarm_type sensor_timeout_alarm = {0,0};
alarm_type sensor_reset_alarm = {0,0};
alarm_type display_timeout_alarm = {0,0};
alarm_type unit_change_timeout_alarm = {0,0};
alarm_type button_press_reset_alarm = {0,0};

//globals for measuring the trigger time for the sensors
uint32_t timer_end_us = 0;
uint32_t timer_start_us = 0;
uint32_t sensor_1_trig_time = 0;
uint32_t sensor_2_trig_time = 0;
uint32_t last_button_press = 0;

//function prototypes for the alarm interrupt callback and the gpio interrupt callback
int64_t alarm_callback(alarm_id_t id, void *user_data);
void gpio_callback(uint gpio, uint32_t events);

//functions for checking flags and managing interrupts
void enable_button_interrupt();
void disable_button_interrupt();
void enable_sensor_interrupts();
void disable_sensor_interrupts();

//function to calculate the velocity based on time between the sensors
double calcluateVelocity(uint64_t time_us);
void change_unit();
void control_leds();

//helper functions for speedometer state managers
void check_sensor_interrupts();
void manage_sensor_timeout_alarm();

//protypes for functions to manage the speedometer states and transitions
void transition_to_on_state();
void manage_on_state();
void manage_timing_1_state();
void manage_timing_2_state();
void transition_to_calculate_speed_state();
void manage_calculate_speed_state();
void manage_display_speed_state();
void transition_to_display_speed_sensor_reset_state();
void manage_display_speed_sensor_reset_state();
void change_unit_state();

//these two still need to be implimented
void manage_low_power_state();
void manage_wake_up_state();


/*
*
*  Callback functions for alarm and gpio interrupts
*
*/

//alarms raise flag when triggering alarm with corresponding id
int64_t alarm_callback(alarm_id_t id, void *user_data)
{
  if (id == sensor_timeout_alarm.id)
  {
    sensor_timeout_alarm.flag = 1;
    return 0;
  }
  if (id == sensor_reset_alarm.id)
  {
    sensor_reset_alarm.flag = 1;
    return 0;
  }
  if (id == display_timeout_alarm.id)
  {
    display_timeout_alarm.flag = 1;
    return 0;
  }
  if (id == unit_change_timeout_alarm.id)
  {
    /*will call this function here to store settings, so do not need to manage it in multiple states*/
    //saveSettings();
    unit_change_timeout_alarm.flag = 1;
    return 0;
  }
  if (id == button_press_reset_alarm.id)
  {
    button_press_reset_alarm.flag = 1;
    //this function is here in the callback to reenable button_interrupt here, so it gets reenables regardless of the current state
    enable_button_interrupt();
    return 0;
  }
  return 0;
}

//the callback sets flag for corresponging GPIO pin
void gpio_callback(uint gpio, uint32_t events)
{
  if (gpio == BUTTON_PIN && !gpio_get(gpio))
  {
    button_flag = 1;
    return;
  }
  if (gpio == SENSOR1_PIN && gpio_get(gpio))
  {
    //record time sensor was triggered
    sensor_1_flag = 1;
    sensor_1_trig_time = time_us_32();
    return;
  }
  if (gpio == SENSOR2_PIN && gpio_get(gpio))
  {
    //record time sensor was triggered
    sensor_2_flag = 1;
    sensor_2_trig_time = time_us_32();
    return;
  }
}


/*
*
*  initialization functions for alarms and for gpio
*
*/

//sets the alarm callback function
void init_speedometer_alarms() {
  set_alarm_callback(alarm_callback);
}

//initializes the gpio pins
void init_speedometer_gpio()
{
  //initialize indicator LEDS
  gpio_init(LED1_PIN);
  gpio_init(LED2_PIN);
  gpio_set_dir(LED1_PIN, GPIO_OUT);
  gpio_set_dir(LED2_PIN, GPIO_OUT);

  //initialize sensor pins connected to IR LEDS
  gpio_init(SENSOR1_PIN);
  gpio_init(SENSOR2_PIN);
  gpio_set_dir(SENSOR1_PIN, GPIO_IN);
  gpio_set_dir(SENSOR2_PIN, GPIO_IN);
  //GPIO inputs default to pull down, need to disable pull down
  gpio_disable_pulls(SENSOR1_PIN);
  gpio_disable_pulls(SENSOR2_PIN);

  //initialize button for changinq units, set to pull up
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  //enable the button and sensor interrupts 4-> rising edge, 8-> falling edge, only can have one gpio callbacl
  gpio_set_irq_enabled_with_callback(BUTTON_PIN, 4, true, gpio_callback);
  gpio_set_irq_enabled(SENSOR1_PIN, 8, true);
  gpio_set_irq_enabled(SENSOR2_PIN, 8, true);
}


/*
*
*  Functions for enabling and disabling interrupts, and managing interrupts
*
*/

//enable the interrupt connected to the button
void enable_button_interrupt() {
  gpio_set_irq_enabled(BUTTON_PIN, 4, true);
  button_flag = 0;
}

//disable the interrupt connected to the button
void disable_button_interrupt() {
  gpio_set_irq_enabled(BUTTON_PIN, 4, false);
  button_flag = 0;
}

//enable the sensor interrupts
void enable_sensor_interrupts() {
  gpio_set_irq_enabled(SENSOR1_PIN, 8, true);
  gpio_set_irq_enabled(SENSOR2_PIN, 8, true);
  sensor_2_flag = 0;
  sensor_1_flag = 0;
}

//disable the sensor interrupts
void disable_sensor_interrupts() {
  gpio_set_irq_enabled(SENSOR1_PIN, 8, true);
  gpio_set_irq_enabled(SENSOR2_PIN, 8, true);
  sensor_2_flag = 0;
  sensor_1_flag = 0;
}


/*
*
*  Functions for LED control and for managing units and calculations
*
*/

//Set LEDs based on the status of the sensors
void control_leds()
{
  gpio_put(LED1_PIN, gpio_get(SENSOR1_PIN));
  gpio_put(LED2_PIN, gpio_get(SENSOR2_PIN));
}

//Increment unit, wrap around to beginning after getting to the last unit
void change_unit() {
  unit_index++;
  if(unit_index >= NUM_UNITS) {
    unit_index = 0;
  }
}

//calculate velocity based on velocity constant - currently configured to return m/s
double calcluateVelocity(uint64_t time_us)
{
  return (double)VELOCITY_CONSTANT / time_us;
}


/*
 *
 *  Helper functions for managing states
 *
 */

//check the flags for each sensor interrupt
void check_sensor_interrupts() {
  /*
   * if the first sensor has been triggered,
   * transition to timing_1 state,
   * enable alarm to timeout the sensor,
   * record the start time,
   * and clear flag for second sensor
   */
  if (sensor_1_flag == 1)
  {
    //copy the sensor trigger time into timer_start_us
    timer_start_us = sensor_1_trig_time;
    sensor_2_flag = 0;
    //record the previous state
    previous_speedometer_state = speedometer_state;
    speedometer_state = TIMING_1;
    enable_alarm(&sensor_timeout_alarm, SENSOR_TIMEOUT);
    return;
  }

  /* 
   * if the second sensor has been triggered,
   * transition to timing_2 state,
   * enable alarm to timeout the sensor,
   * record the start time,
   * and clear flag for first sensor
   */
  if (sensor_2_flag == 1)
  {
    //copy the sensor trigger time into timer_start_us
    timer_start_us = sensor_2_trig_time;
    sensor_1_flag = 0;
    //record the previous state
    previous_speedometer_state = speedometer_state;
    speedometer_state = TIMING_2;
    enable_alarm(&sensor_timeout_alarm, SENSOR_TIMEOUT);
    return;
  }
}

//check to see if sensor alarm was triggered and transition to on state
void manage_sensor_timeout_alarm()
{
 if(sensor_timeout_alarm.flag == 1) {
   //transition to previous state, or to on state
   if(previous_speedometer_state == DISPLAY_SPEED_SENSOR_RESET) {
     transition_to_display_speed_sensor_reset_state();
     return;
   }
   transition_to_on_state();
 }
}

/*
 *
 *  Functions to manage the speedometer states and transitions
 *
 */

// This function loops repeatedly, calling corresponding functions for each state
void manage_states()
{
  control_leds();
  switch (speedometer_state)
  {
  case ON:
    manage_on_state();
    break;
  case CHANGE_UNIT:
    //The state machine never enters this state
    break;
  case TIMING_1:
    manage_timing_1_state();
    printf("state is timing1\n\r");
    break;
  case TIMING_2:
    manage_timing_2_state();
    printf("state is timing2\n\r");

    break;
  case CALCULATE_SPEED:
    manage_calculate_speed_state();
    printf("state is calc_speed\n\r");

    break;
  case DISPLAY_SPEED:
    manage_display_speed_state();
    printf("state is display_speed speed is %.2f\n\r",current_speed);

    break;
  case DISPLAY_SPEED_SENSOR_RESET:
    manage_display_speed_sensor_reset_state();

    break;
  case LOW_POWER:
    manage_low_power_state();

    break;
  case WAKE_UP:
    manage_wake_up_state();
 
    break;
  default:
    break;
  }
}

//when transitioning to on state, enable interrupts, and clear display
void transition_to_on_state()
{
  speedometer_state = ON;
  enable_button_interrupt();
  enable_sensor_interrupts();
  clear_display();
}

//In the on state, periodically check for interrupts triggered by button or sensors
void manage_on_state()
{
  control_leds();
  if (button_flag == 1)
  {
    //if button is pressed, transition to display_speed_sensor_reset state
    transition_to_display_speed_sensor_reset_state();
    button_flag = 0;
    return;
  }
  //check and handle sensor interrupts.  Will transition to timing state if triggered
  check_sensor_interrupts();
}

//in timing_1 state, check for sensor_2 flag, and for sensor timeout
void manage_timing_1_state()
{
  if(sensor_2_flag == 1) {
    //if sensor 2 triggered, transition to calculate speed state, and copy time that sensor triggered into timer_end
    timer_end_us = sensor_2_trig_time;
    transition_to_calculate_speed_state();
    return;
  }
  //check for sensor timeout, if sensor has timed out, this function will call transition to on state or prev state
  manage_sensor_timeout_alarm();
}

//in timing_2 state, check for sensor_1 flag, and for sensor timeout
void manage_timing_2_state()
{
  if(sensor_1_flag == 1) {
    //if sensor 1 triggered, transition to calculate speed state, and copy time that sensor triggered into timer_end
    timer_end_us = sensor_1_trig_time;
    transition_to_calculate_speed_state();
    return;
  }
  //check for sensor timeout, if sensor has timed out, this function will call transition to on state or prev state
  manage_sensor_timeout_alarm();
}

//during transition to calculate_speed_state, set sensor_reset_alarm, this prevents sensors from triggering again because
//they were still blocked when the speed was calculated
void transition_to_calculate_speed_state()
{  
  speedometer_state = CALCULATE_SPEED;
  enable_alarm(&sensor_reset_alarm, SENSOR_RESET_TIME);
}

//calculate speed and update display, transition to display_speed state
void manage_calculate_speed_state() {
  uint32_t total_us = timer_end_us - timer_start_us;
  current_speed = calcluateVelocity(total_us);
  display_speed(current_speed, unit_index);
  speedometer_state = DISPLAY_SPEED;
}

//when sensor alarm triggers, transition to display_speed_sensor_reset state
void manage_display_speed_state() {
  if(sensor_reset_alarm.flag == 1) {
    transition_to_display_speed_sensor_reset_state();
  }
}

//enable sensors, and set alarm for display timeout
void transition_to_display_speed_sensor_reset_state()
{
  speedometer_state = DISPLAY_SPEED_SENSOR_RESET;
  enable_sensor_interrupts();
  enable_alarm(&display_timeout_alarm, DISPLAY_TIMEOUT);
}

//on button interrupt, change unit, look for sensor interrupts to return to timing state. On display timeout alarm, return to on state
void manage_display_speed_sensor_reset_state() {
  check_sensor_interrupts();
  if (button_flag == 1)
  {
    change_unit_state();
    button_flag = 0;
    return;
  }
  if(display_timeout_alarm.flag == 1) {
    transition_to_on_state();
    display_timeout_alarm.flag = 0;
  }
}

/*
 * Disable the button interrupt -> it is re-enabled in the alarm callback.  This is a
 * lazy button debounce.
 * 
 * Enable unit_change_timeout alarm after a set time, store the selected unit to flash.
 * This is also handled by the alarm callback.  This prevents a flash write every time
 * the unit is changed
 * 
 * change the unit, and display the speed witrh the updated unit
 * transition to the display_speed_sensor_reset state
 */
void change_unit_state() {
  if(time_us_32() > last_button_press + BUTTON_PRESS_TIMEOUT) {
    last_button_press = time_us_32();
    disable_button_interrupt();
    enable_alarm(&unit_change_timeout_alarm, UNIT_CHANGE_TIMEOUT);
    enable_alarm(&button_press_reset_alarm, BUTTON_PRESS_TIMEOUT);
    change_unit();
    display_speed(current_speed, unit_index);
    transition_to_display_speed_sensor_reset_state();
  }
  button_flag = 0;
}


/*
*
*  These functions still need to be implemented, will enter a low power state after timeout from on state
*  Will wake up on interrupt from button press
*
*/

void manage_low_power_state() {
    //need to implement
    /*
        sensor off
        interrupts off
        set button to wakeup interrupt
        callback goes to startup state
    */
}

void manage_wake_up_state() {
    /*
    call the wakeup functions here
    */
}


