#include <Arduino.h>
#include <esp_task_wdt.h>

struct Config
{
    static constexpr uint32_t kBaudRate = 115200;
    static constexpr uint16_t kTestCycleSeconds = 12;
    static constexpr uint16_t kTestFanOnSeconds = 3;
    static constexpr uint16_t kRealCycleSeconds = 3600;
    static constexpr uint16_t kRealFanOnSeconds = 900;
    static constexpr uint32_t kWdtMarginMs = 10;
    static constexpr bool kIsTestMode = true;
    static constexpr uint16_t kCycleSeconds = kIsTestMode ? kTestCycleSeconds : kRealCycleSeconds;
    static constexpr uint16_t kFanOnSeconds = kIsTestMode ? kTestFanOnSeconds : kRealFanOnSeconds;
    static constexpr uint32_t kWdtTimeoutMs = kFanOnSeconds * 1000 + kWdtMarginMs;
    static constexpr uint8_t kAlarmsPerCycle = kCycleSeconds / kFanOnSeconds;
    static constexpr uint8_t kFanLedPin = 15;
    static constexpr uint8_t kFanPin = 5;
    static constexpr uint32_t kTimerFrequencyHz = 1000000;
    static constexpr uint64_t kTicksPerAlarm = kTimerFrequencyHz * kFanOnSeconds;
};

namespace
{
    volatile uint32_t alarm_index = 0;
    uint32_t printed_index = 0;
    bool timer_started = false;
    RTC_NOINIT_ATTR uint32_t saved_alarm_index;
}

void IRAM_ATTR onTimer()
{
    alarm_index = (alarm_index + 1);
    if (alarm_index >= Config::kAlarmsPerCycle)
    {
        alarm_index = 0;
    }
    saved_alarm_index = alarm_index;
    digitalWrite(Config::kFanLedPin, alarm_index == 0 ? HIGH : LOW);
    digitalWrite(Config::kFanPin, alarm_index == 0 ? HIGH : LOW);
}

void startFanTimer()
{
    if (timer_started)
    {
        Serial.printf("t=%lu ms timer already started, ignored.\n", millis());
        return;
    }
    timer_started = true;

    hw_timer_t *timer = timerBegin(Config::kTimerFrequencyHz);
    timerAttachInterrupt(timer, onTimer);
    timerAlarm(timer, Config::kTicksPerAlarm, true, 0);
}

void setup()
{
    Serial.begin(Config::kBaudRate);

    esp_reset_reason_t reason = esp_reset_reason();

    if (reason != ESP_RST_POWERON && saved_alarm_index < Config::kAlarmsPerCycle)
    {
        alarm_index = saved_alarm_index;
    }
    else
    {
        alarm_index = 0;
    }
    saved_alarm_index = alarm_index;

    pinMode(Config::kFanLedPin, OUTPUT);
    pinMode(Config::kFanPin, OUTPUT);

    digitalWrite(Config::kFanLedPin, alarm_index == 0 ? HIGH : LOW);
    digitalWrite(Config::kFanPin, alarm_index == 0 ? HIGH : LOW);
    printed_index = alarm_index;

    Serial.printf("t=%lu ms start, reset reason %d, alarm %lu, fan %s\n", millis(), reason, alarm_index, alarm_index == 0 ? "ON" : "OFF");

    startFanTimer();

    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = Config::kWdtTimeoutMs, // скільки мс сторож чекає натискання «кнопки пильності»
        .trigger_panic = true                // не натиснули вчасно - перезавантаження (false - лише запис у лог)
    };

    esp_task_wdt_reconfigure(&wdt_config); // застосувати налаштування: сторож уже запущений ядром Arduino
    esp_task_wdt_add(NULL);                // стежимо за поточною задачею
}

void loop()
{
    uint32_t current_index = alarm_index;
    if (current_index != printed_index)
    {
        printed_index = current_index;
        esp_task_wdt_reset(); // даємо дані
        Serial.printf("t=%lu ms alarm %lu, fan %s\n", millis(), current_index, current_index == 0 ? "ON" : "OFF");
    }

    if (Config::kIsTestMode && Serial.available() > 0 && Serial.read() == 'h')
    {
        while (true)
        {
            // навмисне зависання: loop() більше не повертається
        }
    }
}