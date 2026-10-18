#include <Arduino.h>

struct Config
{
    static constexpr uint8_t kLedPin = 2;
    static constexpr uint8_t kButtonPin = 16;
    static constexpr uint32_t kBlinkMs = 500;
    static constexpr uint32_t kDebounceMs = 50;
    static constexpr uint32_t kStatsIters = 1000;
    static constexpr uint32_t kBaud = 115200;
};

enum class LedState : uint8_t
{
    Off,
    On
};

enum class Mode : uint8_t
{
    Blink,
    AlwaysOn,
    AlwaysOff
};

class Led
{
public:
    explicit Led(uint8_t pin) : pin_(pin) {}
    void init()
    {
        pinMode(pin_, OUTPUT);
        set(LedState::Off);
    }
    void set(LedState s)
    {
        state_ = s;
        digitalWrite(pin_, s == LedState::On ? HIGH : LOW);
    }
    void toggle()
    {
        set(state_ == LedState::On ? LedState::Off : LedState::On);
    }

private:
    uint8_t pin_;
    LedState state_ = LedState::Off;
};

namespace
{
    Led led(Config::kLedPin);
    volatile bool button_event = false;
}

void IRAM_ATTR buttonIsr()
{
    button_event = true;
}

void setup()
{
    Serial.begin(Config::kBaud);
    led.init();
    pinMode(Config::kButtonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Config::kButtonPin), buttonIsr, FALLING);
    Serial.println("Blink -> AlwaysOn -> AlwaysOff");
}

void loop()
{
    const uint32_t now_ms = millis();

    // блимання без delay
    static uint32_t next_toggle_ms = 0;
    static Mode mode = Mode::Blink;
    if (mode == Mode::Blink && now_ms >= next_toggle_ms)
    {
        next_toggle_ms = now_ms + Config::kBlinkMs;
        led.toggle();
    }

    // кнопка: debounce і перемикання режимів
    static uint32_t last_accepted_ms = 0;
    if (button_event)
    {
        button_event = false;
        const bool still_pressed = digitalRead(Config::kButtonPin) == LOW;
        if (still_pressed && now_ms - last_accepted_ms >= Config::kDebounceMs)
        {
            last_accepted_ms = now_ms;
            switch (mode)
            {
            case Mode::Blink:
                mode = Mode::AlwaysOn;
                led.set(LedState::On);
                break;
            case Mode::AlwaysOn:
                mode = Mode::AlwaysOff;
                led.set(LedState::Off);
                break;
            case Mode::AlwaysOff:
                mode = Mode::Blink;
                break;
            }
            Serial.printf("mode -> %d\n", static_cast<int>(mode));
        }
    }

    // час ітерації superloop
    static uint32_t iter = 0;
    static uint32_t t_prev_us = micros();
    static uint64_t acc_us = 0;
    static uint32_t max_us = 0;
    const uint32_t t_now_us = micros();
    const uint32_t dt = t_now_us - t_prev_us;
    t_prev_us = t_now_us;
    acc_us += dt;
    if (dt > max_us)
        max_us = dt;
    if (++iter >= Config::kStatsIters)
    {
        Serial.printf("loop: avg %.1f us, max %u us\n",
                      static_cast<double>(acc_us) / iter, max_us);
        iter = 0;
        acc_us = 0;
        max_us = 0;
    }
}