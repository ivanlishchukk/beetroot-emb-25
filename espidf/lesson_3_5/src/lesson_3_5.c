#include <stdio.h>
#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main(void)
{

  ledc_timer_config_t servo_rate_config = {
      .clk_cfg = LEDC_AUTO_CLK,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .freq_hz = 50,
      .duty_resolution = LEDC_TIMER_14_BIT,
      .timer_num = LEDC_TIMER_2,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&servo_rate_config));

  ledc_channel_config_t servo_channel_config = {
      .channel = LEDC_CHANNEL_2,
      .duty = 0,
      .gpio_num = 15,
      .hpoint = 0,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_sel = LEDC_TIMER_2,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&servo_channel_config));

  adc_oneshot_unit_handle_t adc_handle_number;
  adc_oneshot_unit_init_cfg_t adc_unit_conf_1 = {.unit_id = ADC_UNIT_1};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_conf_1, &adc_handle_number));

  adc_oneshot_chan_cfg_t monitor_channel_5 = {
      .atten = ADC_ATTEN_DB_12, // атенюація
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_number, ADC_CHANNEL_5,
                                             &monitor_channel_5));

  while (1)
  {
    int raw_value;

    ESP_ERROR_CHECK(
        adc_oneshot_read(adc_handle_number, ADC_CHANNEL_5, &raw_value));

    double knob_angle = ((double)raw_value * 180) / 2370;

    double servo_angle = knob_angle;

    if (servo_angle > 180)
    {
      servo_angle = 180;
    }

    int pulse_us = 2500 - ((double)servo_angle * 1950) / 180;

    int duty = ((double)pulse_us / 20000) * 16384;

    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty));
    ESP_ERROR_CHECK(
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

    printf(
        "RAW = %d, Angle = %.1f, Servo = %.1f, Pulse = %d, Duty = %d\n",
        raw_value, knob_angle, servo_angle, pulse_us,
        duty);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}