#include <stdio.h> // дає printf
#include <freertos/FreeRTOS.h> // інструмент для пауз
#include <freertos/task.h>           // дає vTaskDelay
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>

void app_main (void) {

  adc_oneshot_unit_init_cfg_t adc_config = {
      .unit_id = ADC_UNIT_1,
  }; // заповнили анкету для АЦП

  adc_oneshot_unit_handle_t adc_handle_1; // номер для АЦП (скринька)
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle_1)); // додали перевірку на помилки і підключили АЦП

  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  }; // заповнили анкету для каналу

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_1, ADC_CHANNEL_4, &config)); //підключили канал

  int new_chars[8] = {0}; // створили масив, задали усі 8 значень як нулі
  int n = sizeof(new_chars) / sizeof(new_chars[0]);
  double avg_boundary = 3257;

  int run_cycle = 0;

  while (1) {
    int cage1;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_1, ADC_CHANNEL_4,
                                     &cage1)); // вимірюємо і друкуємо число
    new_chars[7] = new_chars[6];
    new_chars[6] = new_chars[5];
    new_chars[5] = new_chars[4];
    new_chars[4] = new_chars[3];
    new_chars[3] = new_chars[2];
    new_chars[2] = new_chars[1];
    new_chars[1] = new_chars[0];
    new_chars[0] = cage1;

    // для розрахунку середнього значення оголошуємо змінну і одразу ж суму усіх
    // чисел у комірках ділимо на їх кількість (тобто на 8)
    // середнє змінюється 10 разів на секунду, так як числа змінюються у
    // комірках

    int new_chars_sum = 0;
    for (int i = 0; i < n; i++) {
      new_chars_sum += new_chars[i];
    }
    
    double avg_value = (double)new_chars_sum / n;

    run_cycle = run_cycle + 1;
    const char *result;
    if (run_cycle >= 8) {
      if (avg_value < avg_boundary) {
      result = "ТЕМНО";
      } else {
        result = "СВІТЛО";
      }
    } else {
      result = "LOADING...";
    };
      
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("c0=%d, c1=%d, c2=%d, c3=%d, c4=%d, c5=%d, c6=%d, c7=%d\n",
           new_chars[0], new_chars[1], new_chars[2], new_chars[3], new_chars[4],
           new_chars[5], new_chars[6], new_chars[7]);
    printf("Середнє арифметичне: %.2f\n", avg_value);
    printf("Результат: %s\n", result);  }
}