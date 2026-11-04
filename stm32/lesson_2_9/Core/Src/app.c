   #include "main.h"
   #include "app.h"

   #define BLINK_SLOW_MS 500U
   #define BLINK_FAST_MS 100U

   static uint32_t last_toggle_ms = 0;

   void setup(void)
   {
       HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
   }

   void loop(void)
   {
       uint32_t now_ms = HAL_GetTick();
       uint32_t period_ms = (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) ? BLINK_FAST_MS : BLINK_SLOW_MS;

       if (now_ms - last_toggle_ms >= period_ms) {
           HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
           last_toggle_ms = now_ms;
       }
   }