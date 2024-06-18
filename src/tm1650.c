#include <string.h>

#include "tm1650.h"
#include "tm1650_registers.h"

static const char *TAG = "tm1650";

/**
 * @brief Macro for safely freeing memory.
 *        This macro checks if the pointer is not NULL before calling the free function.
 *        After freeing the memory, it sets the pointer to NULL to prevent dangling pointer issues.
 *        This helps in avoiding double free errors and improves the safety of memory management.
 * @param ptr Pointer to the memory to be freed.
 */
#define FREE(ptr)   \
    if (ptr)        \
    {               \
        free(ptr);  \
        ptr = NULL; \
    }

struct tm1650
{
    tm1650_config_t *config;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_mode_handle;
    i2c_master_dev_handle_t dev_dig1_handle;
    i2c_master_dev_handle_t dev_dig2_handle;
    i2c_master_dev_handle_t dev_dig3_handle;
    i2c_master_dev_handle_t dev_dig4_handle;
    bool initialized;
};

ESP_EVENT_DEFINE_BASE(TM1650_EVENTS);

static esp_err_t tm1650_clone_config(tm1650_config_t *config, tm1650_config_t **result)
{
    tm1650_config_t *_clone_config = NULL;

    _clone_config = calloc(1, sizeof(tm1650_config_t));
    if (!_clone_config)
        return ESP_ERR_NO_MEM;

    memcpy(_clone_config, config, sizeof(tm1650_config_t));

    // defaults
    _clone_config->i2c.clock_speed_hz = config->i2c.clock_speed_hz == 0 ? TM1650_DEFAULT_I2C_CLOCK_SPEED_HZ : config->i2c.clock_speed_hz;

    *result = _clone_config;

    return ESP_OK;
}

esp_err_t tm1650_create(tm1650_config_t *config, tm1650_handle_t *out_tm1650, i2c_master_bus_handle_t *bus_handle)
{
    if (!config || !out_tm1650)
    {
        return ESP_ERR_INVALID_ARG;
    }

    tm1650_handle_t tm1650 = NULL;

    tm1650 = calloc(1, sizeof(struct tm1650));
    if (!tm1650)
    {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(tm1650_clone_config(config, &(tm1650->config)));

    // create bus if bus_handle is null
    if (!bus_handle)
    {
        bus_handle = malloc(sizeof(i2c_master_bus_handle_t));
        i2c_master_bus_config_t i2c_mst_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = tm1650->config->i2c.port,
            .scl_io_num = tm1650->config->i2c.scl_gpio,
            .sda_io_num = tm1650->config->i2c.sda_gpio,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, bus_handle));
    }

    // mode register
    i2c_device_config_t dev_cfg_mode = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TM1650_I2C_MODE_ADDRESS,
        .scl_speed_hz = tm1650->config->i2c.clock_speed_hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg_mode, &tm1650->dev_mode_handle));

    // DIG1 register
    i2c_device_config_t dev_cfg_dig1 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TM1650_I2C_DIG1_ADDRESS,
        .scl_speed_hz = tm1650->config->i2c.clock_speed_hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg_dig1, &tm1650->dev_dig1_handle));

    // DIG2 register
    i2c_device_config_t dev_cfg_dig2 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TM1650_I2C_DIG2_ADDRESS,
        .scl_speed_hz = tm1650->config->i2c.clock_speed_hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg_dig2, &tm1650->dev_dig2_handle));

    // DIG3 register
    i2c_device_config_t dev_cfg_dig3 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TM1650_I2C_DIG3_ADDRESS,
        .scl_speed_hz = tm1650->config->i2c.clock_speed_hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg_dig3, &tm1650->dev_dig3_handle));

    // DIG4 register
    i2c_device_config_t dev_cfg_dig4 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TM1650_I2C_DIG4_ADDRESS,
        .scl_speed_hz = tm1650->config->i2c.clock_speed_hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg_dig4, &tm1650->dev_dig4_handle));

    tm1650->bus_handle = *bus_handle;
    *out_tm1650 = tm1650; // TODO 怎么初始化这个玩意？

    return ESP_OK;
}

esp_err_t tm1650_mode(tm1650_handle_t tm1650, uint8_t mode)
{
    if (!tm1650)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_ERROR_CHECK(i2c_master_transmit(tm1650->dev_mode_handle, &mode, sizeof(uint8_t), -1));
    return ESP_OK;
}

esp_err_t tm1650_display(tm1650_handle_t tm1650, uint16_t number, bool showColon)
{
    if (!tm1650)
    {
        return ESP_ERR_INVALID_ARG;
    }
    // if (number > 9999)
    // {
    //     return ESP_ERR_INVALID_ARG;
    // }

    const uint8_t seg[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    const uint8_t dot = 0x80;

    uint8_t num[4];
    for (int i = 4; i > 0;)
    {
        uint8_t dig = number % 10;
        num[--i] = seg[dig];
        number /= 10;
    }
    if (showColon)
    {
        num[1] |= dot;
    }

    ESP_ERROR_CHECK(i2c_master_transmit(tm1650->dev_dig1_handle, &num[0], sizeof(uint8_t), -1));
    ESP_ERROR_CHECK(i2c_master_transmit(tm1650->dev_dig2_handle, &num[1], sizeof(uint8_t), -1));
    ESP_ERROR_CHECK(i2c_master_transmit(tm1650->dev_dig3_handle, &num[2], sizeof(uint8_t), -1));
    ESP_ERROR_CHECK(i2c_master_transmit(tm1650->dev_dig4_handle, &num[3], sizeof(uint8_t), -1));

    return ESP_OK;
}

esp_err_t tm1650_destroy(tm1650_handle_t tm1650)
{
    if (!tm1650)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(i2c_del_master_bus(tm1650->bus_handle));

    FREE(tm1650->config);
    FREE(tm1650)

    return ESP_OK;
}
