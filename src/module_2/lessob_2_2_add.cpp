#include <Arduino.h>

struct Config
{
    static constexpr uint8_t kPotPin = 4; // ADC1, середня ніжка потенціометра
    static constexpr uint8_t kPwmPin = 5;
    static constexpr uint32_t kPeriodUs = 1000; // 1000 мкс = 1 кГц
    static constexpr uint8_t kAdcEvery = 50;    // читати потенціометр раз на 50 періодів
    static constexpr uint32_t kPrintMs = 500;
    static constexpr uint32_t kBaud = 115200;
};

void setup()
{
    Serial.begin(Config::kBaud);
    analogReadResolution(12);
    pinMode(Config::kPwmPin, OUTPUT);
}

void loop()
{
    static uint32_t t_on_us = 0;
    static uint8_t cnt = 0;

    // ADC читаємо не щоперіоду: одне читання 80 мкс і зіпсувало б форму сигналу
    if (++cnt >= Config::kAdcEvery)
    {
        cnt = 0;
        const int raw = analogRead(Config::kPotPin);        // 0..4095
        t_on_us = (uint32_t)raw * Config::kPeriodUs / 4095; // 0..1000 мкс
    }

    // один період ШІМ
    if (t_on_us > 0)
    {
        digitalWrite(Config::kPwmPin, HIGH);
        delayMicroseconds(t_on_us);
    }
    if (t_on_us < Config::kPeriodUs)
    {
        digitalWrite(Config::kPwmPin, LOW);
        delayMicroseconds(Config::kPeriodUs - t_on_us);
    }

    // телеметрія раз на пів секунди (трохи розтягує один період - для ока непомітно)
    static uint32_t last_print = 0;
    if (millis() - last_print >= Config::kPrintMs)
    {
        last_print = millis();
        Serial.printf("duty = %3u %%  (t_on = %4u us)\n",
                      (unsigned)(t_on_us * 100 / Config::kPeriodUs), (unsigned)t_on_us);
    }
}