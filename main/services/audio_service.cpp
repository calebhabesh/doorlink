#include "services/audio_service.hpp"

#include <algorithm>
#include <cstring>

#include "board_pins.h"
#include "audio_bus.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "AudioService";
constexpr std::uint32_t kSampleRateHz = 16000;
constexpr std::uint16_t kChannels = 1;
constexpr std::uint16_t kBitsPerSample = 16;
constexpr std::size_t kWavHeaderSize = 44;

void put_u16(std::uint8_t *dst, std::uint16_t value)
{
    dst[0] = static_cast<std::uint8_t>(value);
    dst[1] = static_cast<std::uint8_t>(value >> 8);
}

void put_u32(std::uint8_t *dst, std::uint32_t value)
{
    dst[0] = static_cast<std::uint8_t>(value);
    dst[1] = static_cast<std::uint8_t>(value >> 8);
    dst[2] = static_cast<std::uint8_t>(value >> 16);
    dst[3] = static_cast<std::uint8_t>(value >> 24);
}

void write_wav_header(std::uint8_t *header, std::uint32_t pcm_bytes)
{
    std::memcpy(header, "RIFF", 4);
    put_u32(header + 4, pcm_bytes + 36);
    std::memcpy(header + 8, "WAVEfmt ", 8);
    put_u32(header + 16, 16);
    put_u16(header + 20, 1);
    put_u16(header + 22, kChannels);
    put_u32(header + 24, kSampleRateHz);
    put_u32(header + 28, kSampleRateHz * kChannels * (kBitsPerSample / 8));
    put_u16(header + 32, kChannels * (kBitsPerSample / 8));
    put_u16(header + 34, kBitsPerSample);
    std::memcpy(header + 36, "data", 4);
    put_u32(header + 40, pcm_bytes);
}

std::int16_t mic_sample_to_pcm16(std::int32_t raw)
{
    // The ICS-43434 drives signed 24-bit data in the left 32-bit I2S slot.
    // Four times digital gain keeps normal speech useful without clipping taps.
    const std::int32_t amplified = raw >> 14;
    return static_cast<std::int16_t>(
        std::clamp(amplified, static_cast<std::int32_t>(INT16_MIN),
                   static_cast<std::int32_t>(INT16_MAX)));
}
}  // namespace

RecordedAudio::~RecordedAudio()
{
    reset();
}

void RecordedAudio::reset()
{
    if (data_) {
        heap_caps_free(data_);
        data_ = nullptr;
    }
    size_ = 0;
}

AudioService::~AudioService()
{
    stop();
}

esp_err_t AudioService::start_microphone()
{
    if (rx_channel_) {
        return ESP_OK;
    }

    if (!audio_bus_acquire(pdMS_TO_TICKS(3000))) {
        return ESP_ERR_TIMEOUT;
    }
    owns_audio_bus_ = true;
    gpio_set_level(AMP_EN_PIN, 0);
    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&channel_config, nullptr, &rx_channel_);
    if (err != ESP_OK) {
        stop();
        return err;
    }

    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kSampleRateHz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SCK,
            .ws = I2S_AUDIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC_SD,
            .invert_flags = {},
        },
    };
    config.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    err = i2s_channel_init_std_mode(rx_channel_, &config);
    if (err == ESP_OK) {
        err = i2s_channel_enable(rx_channel_);
    }
    if (err != ESP_OK) {
        stop();
        return err;
    }

    ESP_LOGI(kTag, "Visitor microphone active: 16 kHz, left-slot 24-bit source");
    return ESP_OK;
}

esp_err_t AudioService::record_greeting(RecordedAudio &audio,
                                        std::uint32_t duration_ms)
{
    audio.reset();
    if (duration_ms == 0 || duration_ms > 10000) {
        return ESP_ERR_INVALID_ARG;
    }

    const std::size_t target_samples =
        (static_cast<std::size_t>(kSampleRateHz) * duration_ms) / 1000;
    const std::size_t pcm_bytes = target_samples * sizeof(std::int16_t);
    auto *wav = static_cast<std::uint8_t *>(heap_caps_malloc(
        kWavHeaderSize + pcm_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!wav) {
        wav = static_cast<std::uint8_t *>(malloc(kWavHeaderSize + pcm_bytes));
    }
    if (!wav) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = start_microphone();
    if (err != ESP_OK) {
        heap_caps_free(wav);
        return err;
    }

    auto *pcm = reinterpret_cast<std::int16_t *>(wav + kWavHeaderSize);
    std::size_t samples_written = 0;

    while (samples_written < target_samples) {
        std::size_t bytes_read = 0;
        err = i2s_channel_read(rx_channel_, sample_slots_, sizeof(sample_slots_),
                               &bytes_read, pdMS_TO_TICKS(250));
        if (err != ESP_OK) {
            ESP_LOGE(kTag, "Microphone read failed: %s", esp_err_to_name(err));
            break;
        }
        const std::size_t frames = bytes_read / (sizeof(std::int32_t) * 2);
        const std::size_t copy_frames =
            std::min(frames, target_samples - samples_written);
        for (std::size_t frame = 0; frame < copy_frames; ++frame) {
            pcm[samples_written++] =
                mic_sample_to_pcm16(sample_slots_[frame * 2]);
        }
    }

    stop();
    if (err != ESP_OK || samples_written == 0) {
        heap_caps_free(wav);
        return err == ESP_OK ? ESP_FAIL : err;
    }

    const std::uint32_t recorded_pcm_bytes =
        static_cast<std::uint32_t>(samples_written * sizeof(std::int16_t));
    write_wav_header(wav, recorded_pcm_bytes);
    audio.data_ = wav;
    audio.size_ = kWavHeaderSize + recorded_pcm_bytes;
    ESP_LOGI(kTag, "Visitor greeting recorded: %u ms, %u-byte WAV",
             static_cast<unsigned>((samples_written * 1000) / kSampleRateHz),
             static_cast<unsigned>(audio.size_));
    return ESP_OK;
}

void AudioService::stop()
{
    if (rx_channel_) {
        (void)i2s_channel_disable(rx_channel_);
        (void)i2s_del_channel(rx_channel_);
        rx_channel_ = nullptr;
    }

    gpio_reset_pin(I2S_AUDIO_WS);
    gpio_set_direction(I2S_AUDIO_WS, GPIO_MODE_OUTPUT);
    gpio_set_level(I2S_AUDIO_WS, 0);
    gpio_reset_pin(I2S_AUDIO_SCK);
    gpio_set_direction(I2S_AUDIO_SCK, GPIO_MODE_OUTPUT);
    gpio_set_level(I2S_AUDIO_SCK, 0);
    gpio_reset_pin(I2S_SPK_SD);
    gpio_set_direction(I2S_SPK_SD, GPIO_MODE_OUTPUT);
    gpio_set_level(I2S_SPK_SD, 0);
    gpio_reset_pin(I2S_MIC_SD);
    gpio_set_direction(I2S_MIC_SD, GPIO_MODE_INPUT);
    gpio_set_pull_mode(I2S_MIC_SD, GPIO_PULLDOWN_ONLY);
    if (owns_audio_bus_) {
        audio_bus_release();
        owns_audio_bus_ = false;
    }
}

}  // namespace doorbell
