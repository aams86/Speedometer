#include <stdio.h>
#include "pico/stdlib.h"
#include "speedometerStateHandlers.h"
#include "speedometerDisplay.h"

int main()
{
  // Initialize chosen serial port
  stdio_init_all();
  init_speedometer_alarms();
  init_display();
  init_speedometer_gpio();
  // Loop forever
  while (true)
  {
    manage_states();
  }
}

