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
    uint32_t last_accept_ms = 0;
}

void IRAM_ATTR onButtonFalling()
{
    isr_count++;
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
        uint32_t current_ms = millis();
        if ((current_ms - last_accept_ms) >= Config::kDebounceMs)
        {
            accepted_count++;
            last_accept_ms = current_ms;
            Serial.printf("ISR count: %u\n", seen_count);
            Serial.printf("Accepted count: %u\n", accepted_count);
        }
        else
        {
            Serial.printf("ISR count: %u\n", seen_count);
            Serial.printf("Rejected count: %u\n", (seen_count - accepted_count));
        }
    }
}