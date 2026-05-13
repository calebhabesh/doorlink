#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "i2c_scanner";

// Update these pins if you wired the camera's I2C differently
#define I2C_MASTER_SCL_IO           39      // CAM_PIN_SIOC
#define I2C_MASTER_SDA_IO           40      // CAM_PIN_SIOD
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C Master");
    ESP_ERROR_CHECK(i2c_master_init());

    ESP_LOGI(TAG, "Scanning I2C bus...");
    int devices_found = 0;

    for (int address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device found at address 0x%02X", address);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        ESP_LOGE(TAG, "No I2C devices found. The camera logic core is unresponsive.");
    } else {
        ESP_LOGI(TAG, "Scan complete. %d device(s) found.", devices_found);
    }
}
