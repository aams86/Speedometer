#include "speedometerMemory.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include <hardware/sync.h>

#define ADDRESS 0x1009F000
#define SIZE 4096
unsigned char buffer[SIZE];

int previous_value = 0;

//functions for flash storage, need to test and implement 
int lookUpSettings()
{
  previous_value = (int)((*(uint32_t *)(ADDRESS + 0)) >> 0) & 0xFF;
  return previous_value;
}

int saveSettings(uint32_t current_value)
{
  if(current_value != previous_value) {
    uint32_t current_interrupts = save_and_disable_interrupts();
    flash_range_erase(ADDRESS - XIP_BASE, SIZE);
    flash_range_program(ADDRESS - XIP_BASE, buffer, SIZE);
    restore_interrupts(current_interrupts);
  }
    return 0;
}