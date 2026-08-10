#include "audio_bus.h"

#include "freertos/semphr.h"

static SemaphoreHandle_t s_audio_bus_mutex;
static StaticSemaphore_t s_audio_bus_storage;
static portMUX_TYPE s_audio_bus_init_lock = portMUX_INITIALIZER_UNLOCKED;

static SemaphoreHandle_t audio_bus_mutex(void)
{
    taskENTER_CRITICAL(&s_audio_bus_init_lock);
    if (!s_audio_bus_mutex) {
        s_audio_bus_mutex = xSemaphoreCreateMutexStatic(&s_audio_bus_storage);
    }
    taskEXIT_CRITICAL(&s_audio_bus_init_lock);
    return s_audio_bus_mutex;
}

bool audio_bus_acquire(TickType_t timeout_ticks)
{
    SemaphoreHandle_t mutex = audio_bus_mutex();
    return mutex && xSemaphoreTake(mutex, timeout_ticks) == pdTRUE;
}

void audio_bus_release(void)
{
    if (s_audio_bus_mutex) {
        xSemaphoreGive(s_audio_bus_mutex);
    }
}
