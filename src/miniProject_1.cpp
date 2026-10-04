#include <Arduino.h>

#define LDR_PIN 4   // вхід ADC, середня точка дільника
#define RELAY_PIN 5 // вихід на базу VT1 через R3

// Пороги з моїх вимірів ДЗ по АЦП: накритий ~2070, кімната ~3367.
// Зазор між порогами = гістерезис: у сутінках значення тремтить
// навколо порога, і без зазору реле клацало б безперервно.
#define THRESHOLD_DARK 2100  // нижче - вважаємо, що темно
#define THRESHOLD_LIGHT 3500 // вище - вважаємо, що світло

bool relay_on = false; // поточний стан, щоб писати в пін лише при зміні

void setup()
{
    Serial.begin(115200);
    analogReadResolution(12);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // реле вимкнене на старті
    Serial.println("ADC  | стан реле");
    Serial.println("-----+-----------");
}

void loop()
{
    int adc = analogRead(LDR_PIN);

    if (adc < THRESHOLD_DARK && !relay_on)
    { // темнішає
        relay_on = true;
        digitalWrite(RELAY_PIN, HIGH);
        Serial.printf("%4d | ON  (темно)\n", adc);
    }
    else if (adc > THRESHOLD_LIGHT && relay_on)
    { // світлішає
        relay_on = false;
        digitalWrite(RELAY_PIN, LOW);
        Serial.printf("%4d | OFF (світло)\n", adc);
    }
    // між порогами нічого не робимо - гістерезис

    delay(100);
}