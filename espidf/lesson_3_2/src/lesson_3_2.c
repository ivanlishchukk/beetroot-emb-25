#include <stdio.h>
#include <esp_adc/adc_oneshot.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#define V_MAX 3100

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

  adc_cali_curve_fitting_config_t u_cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .chan = ADC_CHANNEL_5,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  adc_cali_handle_t u_cali_num;
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&u_cali_config, &u_cali_num));

  printf("RAW U_manual(mV) U_cali(mV) Error(%%)\n");

  while (1) {
      
    int raw_value;
    ESP_ERROR_CHECK(
        adc_oneshot_read(adc_handle_number, ADC_CHANNEL_5, &raw_value));
    printf("%d ", raw_value);

    double u_manual;
    u_manual = (double)raw_value * V_MAX / 4096;
    printf("%.1f ", u_manual);

    int u_cali;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(u_cali_num, raw_value, &u_cali));
    printf("%d ", u_cali);

    if (u_cali == 0) {
      printf("-\n");
    } else {
      double error_percentage = (double)(u_manual - u_cali) / u_cali * 100;
      printf("%.2f\n", error_percentage);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  };
};