#include <Arduino.h>

struct Config
{
    static constexpr uint16_t kSetupTimeHourSeconds = 12;
    static constexpr uint16_t kSetupTimeMinuteSeconds = 3;
    static constexpr uint16_t kTimeHourSeconds = 3600;
    static constexpr uint16_t kTimeMinuteSeconds = 900;
    static constexpr uint8_t kDeviceStatus = 0;
    static constexpr bool kIsSetup = true;
    static constexpr uint16_t kTimeHourMode = kIsSetup ? kSetupTimeHourSeconds : kTimeHourSeconds;
    static constexpr uint16_t kTimeMinuteMode = kIsSetup ? kSetupTimeMinuteSeconds : kTimeMinuteSeconds;
    static constexpr uint8_t kDCallCount = kTimeHourMode / kTimeMinuteMode;
    static constexpr uint8_t kWindPin = 15;
    static constexpr uint32_t kFrequency = 1000000;
    static constexpr uint64_t kTicks = kFrequency * kTimeMinuteMode;
};

namespace
{
    uint8_t actual_call_count = 0;
}

void IRAM_ATTR onTimer()
{
    actual_call_count = (actual_call_count + 1);
    if (actual_call_count >= Config::kDCallCount)
    {
        actual_call_count = 0;
    }
    digitalWrite(Config::kWindPin, actual_call_count == 0 ? HIGH : LOW);
}

void setup()
{
    pinMode(Config::kWindPin, OUTPUT);
    digitalWrite(Config::kWindPin, HIGH);
    hw_timer_t *timer = timerBegin(Config::kFrequency);
    timerAttachInterrupt(timer, onTimer);
    timerAlarm(timer, Config::kTicks, true, 0);
}

void loop()
{
}