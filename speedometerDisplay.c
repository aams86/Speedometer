#include "speedometerDisplay.h"
#include "ssd1306.h"
#include <stdio.h>
#include <math.h>

const char *units[6] = {"m/s", "mph", "km/h", "in/s", "ft/s","cm/s"};
const float multiplier[6] = {1.0, 2.23694, 3.6, 39.3701, 3.28084, 100};

//initialize the display
void init_display()
{
  SSD1306_Init();
}

//function to clear the display
void clear_display()
{
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_UpdateScreen();
}

//Display speed function
void display_speed(double current_speed, unit_type unit_index) {
  //clear buffer
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  //get current speed value by multiplying speed with unit multiplier
  double current_speed_unit_adjusted = current_speed * multiplier[unit_index];
  //text offsets
  int startPixel = 5;
  int offset = 48;
  //get number of extra digits
  int num_digits = (int)log10(current_speed_unit_adjusted);
  char speed_string[8];
  
  if(current_speed_unit_adjusted < 10) {
    //if value is less than 10, display thousandths place
    offset += 11;
    sprintf(speed_string, "%.3f", current_speed_unit_adjusted);
  } else if(num_digits < 4) {
    //display hundredths place, add space for extra digits
    offset += num_digits * 11; 
    sprintf(speed_string, "%.2f", current_speed_unit_adjusted);
  } else {
    //if value is any bigger (probably impossible), display value as integer
    sprintf(speed_string, "%d", (int)current_speed_unit_adjusted);
    offset = (14 + num_digits * 11); 
  }
  //copy text into screen buffer
  SSD1306_GotoXY(startPixel + offset, 8);
  SSD1306_Puts("speed", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(startPixel + offset + 1, 20);
  SSD1306_Puts("(", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_Puts(units[unit_index], &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_Puts(")", &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(startPixel, 12);
  SSD1306_Puts(speed_string, &Font_11x18, SSD1306_COLOR_WHITE);
  //update the screen
  SSD1306_UpdateScreen();
}
