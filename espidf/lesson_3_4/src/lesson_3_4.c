#include <stdio.h>
#include <esp_timer.h>
#include <driver/ledc.h>
#include <esp_err.h>

int counter = 0;
int row_number = 0;
int freq[37] = {
    587, 880, 880, 698, 659, 659, 659, 659, 698, 587,
    880, 880, 698, 659, 659,
    659, 659, 698, 587, 880, 880, 698, 659, 659,
    659, 659, 698, 587,
    880, 880, 698, 659, 659, 659, 659, 698, 0};
int sound[37] = {    6, 6, 6, 6, 6, 6, 6, 2, 3,
    6, 6, 6, 6, 6, 6, 6, 2, 3,
    6, 6, 6, 6, 6, 6, 6, 2, 3,
    6, 6, 6, 6, 6, 6, 6, 2, 3,
    0};
int silence[37] = {    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    0};
    
int play_flag;

void run_func(void *arg) {

   if (play_flag == 1) {
    return;
   }
   
  printf("CALLBACK %lld\n", esp_timer_get_time());
  printf("ledc_get_freq = %lu\n", ledc_get_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1));
  printf("ledc_get_duty = %lu\n", ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));

  int row_melody_sum = sound[row_number] + silence[row_number];

  counter = counter + 1;

  if (counter == sound[row_number]) {
    printf("OFF\n");
    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    ESP_ERROR_CHECK(
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
  }

  if (counter == row_melody_sum) {
    row_number = row_number + 1;
    counter = 0;
    row_melody_sum = sound[row_number] + silence[row_number];
    if (row_melody_sum != 0) {
      printf("ON, %d, %d, %d\n", freq[row_number], sound[row_number],
             silence[row_number]);
      ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 8192));
      ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
      ESP_ERROR_CHECK(
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq[row_number]));
    }
  } else if (row_melody_sum == 0) {
    printf("THE END!\n");
    play_flag = 1;
  }
  
printf(" %d, %d, %d\n", freq[row_number], sound[row_number], silence[row_number]);
printf("Counter: %d\n", counter);

};

void app_main(void) {
  printf("lesson 3.4 start\n");

  ledc_timer_config_t note_rate_config = {.clk_cfg = LEDC_AUTO_CLK,
                                      .speed_mode = LEDC_LOW_SPEED_MODE,
                                      .freq_hz = freq[0],
                                      .duty_resolution = LEDC_TIMER_14_BIT,
                                      .timer_num = LEDC_TIMER_1,
                                    };
  ESP_ERROR_CHECK(ledc_timer_config(&note_rate_config));

  ledc_channel_config_t note_channel_config = {
    .channel = LEDC_CHANNEL_1,
    .duty = 8192,
    .gpio_num = 18,
    .hpoint = 0,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_sel = LEDC_TIMER_1,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&note_channel_config));

  esp_timer_create_args_t timer_config = {.callback = run_func,
                                          .name = "Timer"};
  esp_timer_handle_t timer_handler;
  ESP_ERROR_CHECK(esp_timer_create(&timer_config, &timer_handler));
  ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, 81000));
};