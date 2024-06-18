#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <esp_event.h>
#include <driver/i2c_master.h>

#define TM1650_I2C_MODE_ADDRESS (0x24)
#define TM1650_I2C_DIG1_ADDRESS (0x34)
#define TM1650_I2C_DIG2_ADDRESS (0x35)
#define TM1650_I2C_DIG3_ADDRESS (0x36)
#define TM1650_I2C_DIG4_ADDRESS (0x37)

#define TM1650_DEFAULT_I2C_CLOCK_SPEED_HZ (100000)

    ESP_EVENT_DECLARE_BASE(TM1650_EVENTS);

    typedef struct tm1650 *tm1650_handle_t;

    typedef struct
    {
        struct
        {
            i2c_port_t port;
            int sda_gpio;
            int scl_gpio;
            int rw_timeout_ms;
            uint32_t clock_speed_hz;
        } i2c;
    } tm1650_config_t;

    esp_err_t tm1650_create(tm1650_config_t *config, tm1650_handle_t *out_tm1650, i2c_master_bus_handle_t *bus_handle);
    esp_err_t tm1650_mode(tm1650_handle_t tm1650, uint8_t mode);
    esp_err_t tm1650_display(tm1650_handle_t tm1650, uint16_t number, bool showColon);
    esp_err_t tm1650_destroy(tm1650_handle_t tm1650);

#ifdef __cplusplus
}
#endif