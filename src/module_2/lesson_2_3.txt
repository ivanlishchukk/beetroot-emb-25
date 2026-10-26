#include <Arduino.h>

enum class LedState : uint8_t
{
    Off,
    On
};

struct Config
{
    static constexpr uint8_t kGreenPin = 17;
    static constexpr uint32_t kGreenPeriodMs = 200;
    static constexpr uint8_t kYellowPin = 16;
    static constexpr uint32_t kYellowPeriodMs = 500;
    static constexpr uint8_t kRedPin = 15;
    static constexpr uint32_t kRedPeriodMs = 1000;
};

class Led
{
public:
    void init(uint8_t pin, uint32_t period_ms)
    {
        pin_ = pin;
        period_ms_ = period_ms;
        pinMode(pin, OUTPUT);
        set(LedState::Off);
    }

    void set(LedState state)
    {
        state_ = state;
        digitalWrite(pin_, state_ == LedState::On ? HIGH : LOW);
    }

    void update(uint32_t now_ms)
    {
        if (now_ms - last_ms_ >= period_ms_)
        {
            set(state_ == LedState::On ? LedState::Off : LedState::On);
            last_ms_ = now_ms;
        }
    }

private:
    uint8_t pin_ = 0;
    uint32_t period_ms_ = 0;
    LedState state_ = LedState::Off;
    uint32_t last_ms_ = 0;
};

namespace
{
    Led green_led;
    Led yellow_led;
    Led red_led;
}

void setup()
{
    green_led.init(Config::kGreenPin, Config::kGreenPeriodMs);
    yellow_led.init(Config::kYellowPin, Config::kYellowPeriodMs);
    red_led.init(Config::kRedPin, Config::kRedPeriodMs);
}

void loop()
{
    uint32_t now_ms = millis(); // глянули на годинник

    green_led.update(now_ms);
    yellow_led.update(now_ms);
    red_led.update(now_ms);
}