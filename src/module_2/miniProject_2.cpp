#include <Arduino.h>

enum class Light : uint8_t
{
    kGreen,
    kGreenBlink,
    kYellow,
    kRed,
    kRedYellow,
    kYellowBlink,
};

enum class Event : uint8_t
{
    kStateTimeout,
    kBlinkTick,
    kButton,
};

struct Config
{
    static constexpr uint32_t kBaudRate = 115200;

    static constexpr uint8_t kGreenPin = 17;
    static constexpr uint8_t kYellowPin = 16;
    static constexpr uint8_t kRedPin = 15;
    static constexpr uint8_t kButtonPin = 4;

    static constexpr uint32_t kGreenMs = 10000;
    static constexpr uint32_t kGreenBlinkMs = 3000;
    static constexpr uint32_t kYellowMs = 3000;
    static constexpr uint32_t kRedMs = 10000;
    static constexpr uint32_t kRedYellowMs = 2000;

    static constexpr uint32_t kBlinkHalfPeriodMs = 500; // миготіння 1 Гц: 500 мс світить, 500 мс ні
    static constexpr uint32_t kDebounceMs = 50;

    static constexpr uint32_t kTimerFrequencyHz = 1000000; // 1 тік = 1 мкс
    static constexpr uint64_t kTicksPerMs = kTimerFrequencyHz / 1000;
};

namespace
{
    volatile bool state_timeout_flag = false;
    volatile bool blink_tick_flag = false;
    volatile bool button_edge_flag = false;

    hw_timer_t *state_timer = nullptr;
    hw_timer_t *blink_timer = nullptr;
    bool state_timer_running = false;

    Light light = Light::kRed;
    bool blink_on = true; // лампа світить

    bool button_pending = false;
    uint32_t last_button_edge_ms = 0;
}

void IRAM_ATTR onStateTimer()
{
    state_timeout_flag = true;
}

void IRAM_ATTR onBlinkTimer()
{
    blink_tick_flag = true;
}

void IRAM_ATTR onButtonFalling()
{
    button_edge_flag = true;
}

const char *lightName(Light state)
{
    switch (state)
    {
    case Light::kGreen:
        return "GREEN";
    case Light::kGreenBlink:
        return "GREEN BLINK";
    case Light::kYellow:
        return "YELLOW";
    case Light::kRed:
        return "RED";
    case Light::kRedYellow:
        return "RED + YELLOW";
    case Light::kYellowBlink:
        return "YELLOW BLINK (unregulated)";
    }
    return "?";
}

uint32_t stateDurationMs(Light state)
{
    switch (state)
    {
    case Light::kGreen:
        return Config::kGreenMs;
    case Light::kGreenBlink:
        return Config::kGreenBlinkMs;
    case Light::kYellow:
        return Config::kYellowMs;
    case Light::kRed:
        return Config::kRedMs;
    case Light::kRedYellow:
        return Config::kRedYellowMs;
    case Light::kYellowBlink:
        return 0;
    }
    return 0;
}

Light nextLight(Light state)
{
    switch (state)
    {
    case Light::kGreen:
        return Light::kGreenBlink;
    case Light::kGreenBlink:
        return Light::kYellow;
    case Light::kYellow:
        return Light::kRed;
    case Light::kRed:
        return Light::kRedYellow;
    case Light::kRedYellow:
        return Light::kGreen;
    case Light::kYellowBlink:
        return Light::kYellowBlink;
    }
    return Light::kRed;
}

bool isBlinking(Light state)
{
    return state == Light::kGreenBlink || state == Light::kYellowBlink;
}

void writeLamps()
{
    bool green = false;
    bool yellow = false;
    bool red = false;

    switch (light)
    {
    case Light::kGreen:
        green = true;
        break;
    case Light::kGreenBlink:
        green = blink_on;
        break;
    case Light::kYellow:
        yellow = true;
        break;
    case Light::kRed:
        red = true;
        break;
    case Light::kRedYellow:
        red = true;
        yellow = true;
        break;
    case Light::kYellowBlink:
        yellow = blink_on;
        break;
    }

    digitalWrite(Config::kGreenPin, green ? HIGH : LOW);
    digitalWrite(Config::kYellowPin, yellow ? HIGH : LOW);
    digitalWrite(Config::kRedPin, red ? HIGH : LOW);
}

void armStateTimer(uint32_t duration_ms)
{
    state_timeout_flag = false;
    timerWrite(state_timer, 0);
    timerAlarm(state_timer, duration_ms * Config::kTicksPerMs, false, 0);
    if (!state_timer_running)
    {
        timerStart(state_timer);
        state_timer_running = true;
    }
}

void stopStateTimer()
{
    if (state_timer_running)
    {
        timerStop(state_timer);
        state_timer_running = false;
    }
    state_timeout_flag = false;
}

void enterLight(Light next)
{
    light = next;

    blink_on = true;
    if (isBlinking(light))
    {
        timerWrite(blink_timer, 0);
        blink_tick_flag = false;
    }

    const uint32_t duration_ms = stateDurationMs(light);
    if (duration_ms > 0)
    {
        armStateTimer(duration_ms);
    }
    else
    {
        stopStateTimer();
    }

    writeLamps();
    Serial.printf("t=%lu ms %s\n", millis(), lightName(light));
}

void handleEvent(Event event)
{
    switch (event)
    {
    case Event::kStateTimeout:
        if (light != Light::kYellowBlink)
        {
            enterLight(nextLight(light));
        }
        break;

    case Event::kBlinkTick:
        if (isBlinking(light))
        {
            blink_on = !blink_on;
            writeLamps();
        }
        break;

    case Event::kButton:
        if (light == Light::kYellowBlink)
        {
            Serial.printf("t=%lu ms button: regulated mode\n", millis());
            enterLight(Light::kYellow);
        }
        else
        {
            Serial.printf("t=%lu ms button: unregulated mode\n", millis());
            enterLight(Light::kYellowBlink);
        }
        break;
    }
}

void updateButton(uint32_t now_ms)
{
    if (button_edge_flag)
    {
        button_edge_flag = false;
        button_pending = true;
        last_button_edge_ms = now_ms;
    }

    if (button_pending && now_ms - last_button_edge_ms >= Config::kDebounceMs)
    {
        button_pending = false;
        if (digitalRead(Config::kButtonPin) == LOW) // досі натиснута - справжнє натискання
        {
            handleEvent(Event::kButton);
        }
        // уже відпущена - брязкіт відпускання, подію ігноруємо
    }
}

void setup()
{
    Serial.begin(Config::kBaudRate);

    pinMode(Config::kGreenPin, OUTPUT);
    pinMode(Config::kYellowPin, OUTPUT);
    pinMode(Config::kRedPin, OUTPUT);
    pinMode(Config::kButtonPin, INPUT_PULLUP);

    state_timer = timerBegin(Config::kTimerFrequencyHz);
    timerAttachInterrupt(state_timer, onStateTimer);
    timerStop(state_timer); // відлік запускає armStateTimer()
    state_timer_running = false;

    blink_timer = timerBegin(Config::kTimerFrequencyHz);
    timerAttachInterrupt(blink_timer, onBlinkTimer);
    timerAlarm(blink_timer, Config::kBlinkHalfPeriodMs * Config::kTicksPerMs, true, 0);

    attachInterrupt(digitalPinToInterrupt(Config::kButtonPin), onButtonFalling, FALLING);

    Serial.printf("t=%lu ms traffic light start\n", millis());
    enterLight(Light::kRed); // старт із заборони руху
}

void loop()
{
    const uint32_t now_ms = millis();

    if (state_timeout_flag)
    {
        state_timeout_flag = false;
        handleEvent(Event::kStateTimeout);
    }

    if (blink_tick_flag)
    {
        blink_tick_flag = false;
        handleEvent(Event::kBlinkTick);
    }

    updateButton(now_ms);
}