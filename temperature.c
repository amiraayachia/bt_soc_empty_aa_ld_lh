/*
 * temperature.c
 *
 *  Created on: 5 mai 2025
 *      Author: amira
 */


#include "em_common.h"
#include "app_assert.h"
#include "app.h"
#include "sl_sensor_rht.h"





uint32_t temp_recup(void){

  uint32_t rh;
  int32_t t;

  sl_sensor_rht_init();
  sl_sensor_rht_get(&rh, &t);
  sl_sensor_rht_deinit();

  return t/100;
}






