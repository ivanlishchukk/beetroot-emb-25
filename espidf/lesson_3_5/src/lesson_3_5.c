#include <stdio.h>
#include <driver/ledc.h>
#include <esp_err.h>

void app_main(void) {
  printf("lesson 3.5 start\n");

  ledc_timer_config_t servo_rate_config = {.clk_cfg = LEDC_AUTO_CLK,
                                      .speed_mode = LEDC_LOW_SPEED_MODE,
                                      .freq_hz = 50,
                                      .duty_resolution = LEDC_TIMER_14_BIT,
                                      .timer_num = LEDC_TIMER_2,
                                    };
  ESP_ERROR_CHECK(ledc_timer_config(&servo_rate_config));

  ledc_channel_config_t servo_channel_config = {
    .channel = LEDC_CHANNEL_2,
    .duty = 1638,
    .gpio_num = 15,
    .hpoint = 0,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_sel = LEDC_TIMER_2,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&servo_channel_config));
}