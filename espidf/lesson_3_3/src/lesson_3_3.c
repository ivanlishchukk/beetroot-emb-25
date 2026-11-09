#include <stdio.h>
#include <esp_adc/adc_oneshot.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>

void app_main(void) {
  adc_oneshot_unit_handle_t adc_handle_number;
  adc_oneshot_unit_init_cfg_t adc_unit_conf_1 = {.unit_id = ADC_UNIT_1};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_conf_1, &adc_handle_number));

  adc_oneshot_chan_cfg_t monitor_channel_5 = {
      .atten = ADC_ATTEN_DB_12, // атенюація
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_number, ADC_CHANNEL_5,
                                             &monitor_channel_5));

  ledc_timer_config_t led_timer_config = {.clk_cfg = LEDC_AUTO_CLK,
                                      .speed_mode = LEDC_LOW_SPEED_MODE,
                                      .freq_hz = 1000,
                                      .duty_resolution = LEDC_TIMER_14_BIT,
                                      .timer_num = LEDC_TIMER_0,
                                    };
  ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));

  ledc_channel_config_t led_channel_config = {
    .channel = LEDC_CHANNEL_0,
    .duty = 8192,
    .gpio_num = 17,
    .hpoint = 0,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_sel = LEDC_TIMER_0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));

  ledc_timer_config_t motor_timer_config = {.clk_cfg = LEDC_AUTO_CLK,
                                      .speed_mode = LEDC_LOW_SPEED_MODE,
                                      .freq_hz = 20000,
                                      .duty_resolution = LEDC_TIMER_11_BIT,
                                      .timer_num = LEDC_TIMER_1,
                                    };
  ESP_ERROR_CHECK(ledc_timer_config(&motor_timer_config));

  ledc_channel_config_t motor_channel_config = {
    .channel = LEDC_CHANNEL_1,
    .duty = 0,
    .gpio_num = 18,
    .hpoint = 0,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_sel = LEDC_TIMER_1,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&motor_channel_config));
  
  printf("RAW LED_duty MOTOR_duty\n");

  while (1) {
      
    int raw_value;
    ESP_ERROR_CHECK(
        adc_oneshot_read(adc_handle_number, ADC_CHANNEL_5, &raw_value));
    printf("%d ", raw_value);

    int led_duty = raw_value*4;
    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, led_duty));
    ESP_ERROR_CHECK(
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    printf("%d ", led_duty);

    int motor_duty = raw_value/2;
    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, motor_duty));
    ESP_ERROR_CHECK(
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));

    printf("%d\n", motor_duty);

    vTaskDelay(pdMS_TO_TICKS(100));
  };
};