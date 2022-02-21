add_executable(speedometer
        speedometer.c
        fonts.c
        ssd1306.c
        speedometerStateHandlers.c
        speedometerAlarms.c
        speedometerDisplay.c
        speedometerMemory.c
        )

# pull in common dependencies
target_link_libraries(speedometer pico_stdlib hardware_i2c hardware_flash hardware_adc)


# create map/bin/hex file etc.
pico_add_extra_outputs(speedometer)

# add url via pico_set_program_url
example_auto_set_url(speedometer)

# Enable usb output, disable uart output
#pico_enable_stdio_usb(${PROJECT_NAME} 0)
#pico_enable_stdio_uart(${PROJECT_NAME} 1)