#include <Arduino.h>

struct Config
{
    static constexpr uint8_t kButtonPin = 4;
    static constexpr uint32_t kBaudRate = 115200;
    static constexpr uint32_t kDebounceMs = 50;
};

namespace
{
    volatile uint32_t isr_count = 0;
    uint32_t seen_count = 0;
    uint32_t accepted_count = 0;
    uint32_t event_ms = 0;
    bool pending = false;
}

void IRAM_ATTR onButtonFalling()
{
    isr_count = isr_count + 1;
}

void setup()
{
    Serial.begin(Config::kBaudRate);
    pinMode(Config::kButtonPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(Config::kButtonPin), onButtonFalling, FALLING);
}

void loop()
{
    uint32_t count = isr_count;
    if (count != seen_count)
    {
        seen_count = count;
        event_ms = millis();
        pending = true;
    }

    if (pending && millis() - event_ms >= Config::kDebounceMs)
    {
        pending = false;
        if (digitalRead(Config::kButtonPin) == LOW)
        {
            accepted_count++;
            Serial.print("Button pressed! Count: ");
            Serial.println(accepted_count);
        }
        else
        {
            Serial.println("Button bounce ignored");
        }
    }
}