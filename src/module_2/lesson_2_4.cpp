#include <Arduino.h>

enum class ButtonState : uint8_t
{
    kReleased,
    kMaybePressed,
    kPressed,
    kMaybeReleased
};

struct Config
{
    static constexpr uint8_t kButtonPin = 4;
    static constexpr uint32_t kBaudRate = 115200;
    static constexpr uint32_t kDebounceMs = 50;
    static constexpr uint32_t kPollMs = 10;
};

namespace
{
    ButtonState state = ButtonState::kReleased;
    uint32_t last_polled_ms = 0;
    uint32_t change_ms = 0;
    uint32_t accepted_count = 0;
}

void setup()
{
    Serial.begin(Config::kBaudRate);
    pinMode(Config::kButtonPin, INPUT);
}

void loop()
{
    uint32_t now_ms = millis();
    if (now_ms - last_polled_ms >= Config::kPollMs)
    {
        last_polled_ms = now_ms;
        uint8_t level = digitalRead(Config::kButtonPin);

        if (state == ButtonState::kReleased)
        {
            if (level == LOW)
            {
                state = ButtonState::kMaybePressed;
                change_ms = now_ms;
            }
        }
        else if (state == ButtonState::kMaybePressed)
        {
            if (level == LOW)
            {
                if (now_ms - change_ms >= Config::kDebounceMs)
                {
                    state = ButtonState::kPressed;
                    accepted_count++;
                    Serial.printf("Button pressed %d times\n", accepted_count);
                }
            }
            else
            {
                state = ButtonState::kReleased;
            }
        }
        else if (state == ButtonState::kPressed)
        {
            if (level == HIGH)
            {
                state = ButtonState::kMaybeReleased;
                change_ms = now_ms;
            }
        }
        else if (state == ButtonState::kMaybeReleased)
        {
            if (level == HIGH)
            {
                if (now_ms - change_ms >= Config::kDebounceMs)
                {
                    state = ButtonState::kReleased;
                }
            }
            else
            {
                state = ButtonState::kPressed;
            }
        }
    }
}