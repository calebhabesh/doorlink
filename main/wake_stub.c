#include "sdkconfig.h"

#if CONFIG_SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC || \
    CONFIG_SMART_DOORBELL_PRODUCTION_APP

#include <stdint.h>

#include "board_pins.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_wake_stub.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc.h"

void wake_stub_install(void);

static inline void wake_stub_set_led(gpio_num_t pin)
{
    /*
     * These LL helpers are always-inline register accesses. Flash is not
     * mapped while the wake stub runs, so do not replace them with the
     * normal GPIO driver functions.
     */
    gpio_ll_set_level(&GPIO, (uint32_t)pin, 1);
    gpio_ll_output_enable(&GPIO, (uint32_t)pin);
    gpio_ll_func_sel(&GPIO, (uint8_t)pin, PIN_FUNC_GPIO);
}

void RTC_IRAM_ATTR esp_wake_deep_sleep(void)
{
    /* Espressif requires the default stub initialization to run first. */
    esp_default_wake_deep_sleep();

    /* EXT0 is the active-low doorbell input on GPIO2. */
    if ((esp_wake_stub_get_wakeup_cause() & RTC_EXT0_TRIG_EN) != 0) {
        wake_stub_set_led(BUTTON_LED_PIN);
        wake_stub_set_led(STATUS_LED_PIN);
    }
}

void wake_stub_install(void)
{
    /* Also creates an explicit reference so this strong weak-symbol override
     * is retained when the component archive is linked. */
    esp_set_deep_sleep_wake_stub(esp_wake_deep_sleep);
}

#endif
