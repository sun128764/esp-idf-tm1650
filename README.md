# esp-idf-tm1650
 
C library for interfacing ESP32 with TM1650 4-dig led display, packaged as ESP-IDF component

## How to use

This directory is an ESP-IDF component. Clone it (or add it as submodule) into `components` directory of the project.

## Example
This is a basic example to display "23:45" on the LED display and set the bright to level 1.
```c
#include "tm1650.h"
#include "tm1650_registers.h"

void app_main()
{
    tm1650_config_t config_1650 = {
        .i2c.port = -1,
        .i2c.scl_gpio = 6,
        .i2c.sda_gpio = 7};

    tm1650_create(&config_1650, &display, NULL);
    ESP_ERROR_CHECK(tm1650_mode(display,TM1650_BRIGHT1));
    ESP_ERROR_CHECK(tm1650_display(display, 2345, true));
    ESP_ERROR_CHECK(tm1650_destroy(display));
}
```

## Detail

### Use your own i2c_master_bus

```c
tm1650_create(&config_1650, &display, &your_i2c_master_handle);
```
