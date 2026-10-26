#include <Arduino.h>

struct Config
{
    static constexpr uint8_t kButtonPin = 4;
    static constexpr uint32_t kBaudRate = 115200;
};

namespace
{
    volatile uint32_t isr_count = 0;
    uint32_t printed_count = 0;
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
    if (count != printed_count)
    {
        Serial.printf("ISR count: %u\n", count);
        printed_count = count;
    }
}