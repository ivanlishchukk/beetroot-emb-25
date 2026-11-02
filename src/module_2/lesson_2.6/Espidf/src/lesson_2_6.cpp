#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_err.h"

enum class RunMode : uint8_t
{
    kAll,
    kRaw,
    kTimeBased,
    kStateBased,
    kPolling,
};

enum class ButtonState : uint8_t
{
    kReleased,
    kMaybePressed,
    kPressed,
    kMaybeReleased,
};

struct Config
{
    static constexpr RunMode kRunMode = RunMode::kAll;
    static constexpr gpio_num_t kButtonPin = GPIO_NUM_4;
    static constexpr int64_t kDebounceUs = 50 * 1000;       // вікно дебаунсу 50 мс
    static constexpr uint32_t kLoopPeriodMs = 10;           // крок циклу й опитування
    static constexpr int64_t kSummaryQuietUs = 3000 * 1000; // підсумок після 3 с тиші
    static constexpr UBaseType_t kEdgeQueueLength = 256;    // черга міток часу з ISR
};

static_assert(pdMS_TO_TICKS(Config::kLoopPeriodMs) > 0, "kLoopPeriodMs is shorter than one FreeRTOS tick");

namespace
{
    constexpr bool kUseIsr = Config::kRunMode != RunMode::kPolling;
    constexpr bool kRunRaw = Config::kRunMode == RunMode::kAll || Config::kRunMode == RunMode::kRaw;
    constexpr bool kRunTimeBased = Config::kRunMode == RunMode::kAll || Config::kRunMode == RunMode::kTimeBased;
    constexpr bool kRunStateBased = Config::kRunMode == RunMode::kAll || Config::kRunMode == RunMode::kStateBased;
    constexpr bool kRunPolling = Config::kRunMode == RunMode::kAll || Config::kRunMode == RunMode::kPolling;

    struct MethodStats
    {
        uint32_t presses = 0;
        int64_t latency_sum_us = 0;
    };

    QueueHandle_t edge_queue = nullptr;
    volatile uint32_t raw_edges = 0;
    volatile uint32_t dropped_edges = 0;

    int64_t last_edge_us = -Config::kDebounceUs;
    int64_t burst_start_us = 0;

    uint32_t printed_raw = 0;

    int64_t last_accepted_us = -Config::kDebounceUs;
    MethodStats time_stats;

    bool state_pending = false;
    MethodStats state_stats;

    ButtonState poll_state = ButtonState::kReleased;
    int64_t poll_change_us = 0;
    MethodStats poll_stats;

    int64_t last_activity_us = 0;
    bool summary_due = false;
}

void IRAM_ATTR onButtonFalling(void *arg)
{
    (void)arg;
    const int64_t now_us = esp_timer_get_time();
    raw_edges = raw_edges + 1;

    BaseType_t higher_priority_woken = pdFALSE;
    if (xQueueSendFromISR(edge_queue, &now_us, &higher_priority_woken) != pdTRUE)
    {
        dropped_edges = dropped_edges + 1;
    }
    if (higher_priority_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

const char *runModeName(RunMode mode)
{
    switch (mode)
    {
    case RunMode::kAll:
        return "all methods";
    case RunMode::kRaw:
        return "task 1: raw";
    case RunMode::kTimeBased:
        return "task 2: time-based";
    case RunMode::kStateBased:
        return "task 3: state-based";
    case RunMode::kPolling:
        return "task 4: polling FSM";
    }
    return "?";
}

bool isButtonPressed()
{
    return gpio_get_level(Config::kButtonPin) == 0;
}

void acceptPress(MethodStats &stats, const char *method, int64_t now_us, int64_t press_start_us)
{
    stats.presses = stats.presses + 1;
    const int64_t latency_us = now_us - press_start_us;
    stats.latency_sum_us = stats.latency_sum_us + latency_us;
    printf("t=%" PRId64 " ms %s PRESS #%" PRIu32 ", latency %" PRId64 " ms\n",
           now_us / 1000, method, stats.presses, latency_us / 1000);
}

int64_t averageLatencyMs(const MethodStats &stats)
{
    return stats.presses == 0 ? 0 : stats.latency_sum_us / stats.presses / 1000;
}

void processEdge(int64_t edge_us)
{
    if (edge_us - last_edge_us >= Config::kDebounceUs)
    {
        burst_start_us = edge_us;
    }
    last_edge_us = edge_us;
    last_activity_us = edge_us;
    summary_due = true;

    if (kRunTimeBased && edge_us - last_accepted_us >= Config::kDebounceUs)
    {
        last_accepted_us = edge_us;
        acceptPress(time_stats, "time-based ", esp_timer_get_time(), burst_start_us);
    }

    if (kRunStateBased)
    {
        state_pending = true;
    }
}

void updateStateBased(int64_t now_us)
{
    if (!state_pending || now_us - last_edge_us < Config::kDebounceUs)
    {
        return;
    }
    state_pending = false;
    if (isButtonPressed())
    {
        acceptPress(state_stats, "state-based", now_us, burst_start_us);
    }
}

void updatePolling(int64_t now_us)
{
    const bool pressed = isButtonPressed();
    switch (poll_state)
    {
    case ButtonState::kReleased:
        if (pressed)
        {
            poll_state = ButtonState::kMaybePressed;
            poll_change_us = now_us;
            last_activity_us = now_us;
            summary_due = true;
        }
        break;
    case ButtonState::kMaybePressed:
        if (!pressed)
        {
            poll_state = ButtonState::kReleased;
        }
        else if (now_us - poll_change_us >= Config::kDebounceUs)
        {
            poll_state = ButtonState::kPressed;
            acceptPress(poll_stats, "polling    ", now_us, kUseIsr ? burst_start_us : poll_change_us);
        }
        break;
    case ButtonState::kPressed:
        if (!pressed)
        {
            poll_state = ButtonState::kMaybeReleased;
            poll_change_us = now_us;
            last_activity_us = now_us;
        }
        break;
    case ButtonState::kMaybeReleased:
        if (pressed)
        {
            poll_state = ButtonState::kPressed;
        }
        else if (now_us - poll_change_us >= Config::kDebounceUs)
        {
            poll_state = ButtonState::kReleased;
        }
        break;
    }
}

void printSummary(int64_t now_us)
{
    printf("=== SUMMARY t=%" PRId64 " ms (%s):", now_us / 1000, runModeName(Config::kRunMode));
    if (kRunRaw)
    {
        printf(" raw edges %" PRIu32 ";", static_cast<uint32_t>(raw_edges));
    }
    if (kRunTimeBased)
    {
        printf(" time-based %" PRIu32 " (avg %" PRId64 " ms);", time_stats.presses, averageLatencyMs(time_stats));
    }
    if (kRunStateBased)
    {
        printf(" state-based %" PRIu32 " (avg %" PRId64 " ms);", state_stats.presses, averageLatencyMs(state_stats));
    }
    if (kRunPolling)
    {
        printf(" polling %" PRIu32 " (avg %" PRId64 " ms);", poll_stats.presses, averageLatencyMs(poll_stats));
    }
    if (kUseIsr)
    {
        printf(" dropped %" PRIu32, static_cast<uint32_t>(dropped_edges));
    }
    printf(" ===\n");
}

extern "C" void app_main(void)
{
    printf("DZ 2.6: %s, debounce %" PRId64 " ms, loop %" PRIu32 " ms\n",
           runModeName(Config::kRunMode), Config::kDebounceUs / 1000, Config::kLoopPeriodMs);

    gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << Config::kButtonPin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // зовнішня підтяжка 10 кОм на бредборді
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = kUseIsr ? GPIO_INTR_NEGEDGE : GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));

    if (kUseIsr)
    {
        edge_queue = xQueueCreate(Config::kEdgeQueueLength, sizeof(int64_t));
        if (edge_queue == nullptr)
        {
            printf("edge queue allocation failed\n");
            return;
        }
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        ESP_ERROR_CHECK(gpio_isr_handler_add(Config::kButtonPin, onButtonFalling, nullptr));
    }

    while (true)
    {
        const int64_t now_us = esp_timer_get_time();

        if (kUseIsr)
        {
            int64_t edge_us = 0;
            while (xQueueReceive(edge_queue, &edge_us, 0) == pdTRUE)
            {
                processEdge(edge_us);
            }
        }

        if (kRunRaw)
        {
            const uint32_t raw = raw_edges;
            if (raw != printed_raw)
            {
                printed_raw = raw;
                printf("t=%" PRId64 " ms raw edges %" PRIu32 "\n", now_us / 1000, raw);
            }
        }

        if (kRunStateBased)
        {
            updateStateBased(now_us);
        }

        if (kRunPolling)
        {
            updatePolling(now_us);
        }

        if (summary_due && now_us - last_activity_us >= Config::kSummaryQuietUs)
        {
            summary_due = false;
            printSummary(now_us);
        }

        vTaskDelay(pdMS_TO_TICKS(Config::kLoopPeriodMs));
    }
}