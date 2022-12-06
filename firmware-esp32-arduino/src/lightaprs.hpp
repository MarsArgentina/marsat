/**
 * @file i2c.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-11-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef LIGHTAPRS_HPP
#define LIGHTAPRS_HPP

#include <Arduino.h>
#include <Wire.h>

typedef struct lightaprs_t
{
  float latitud;
  float longitud;
  float altura;
  float temp_int;
  float temp_ext;
  int32_t presion;

}lightaprs_t;

typedef union
{
  int32_t entero;
  float number;
  uint8_t bytes[4];
} floatunion_t;

void receive_event(int howMany, float *vbat_aprs, float vbat_esp);
void read_last_received(lightaprs_t *lightaprs);
bool new_data_available();
// bool lightaprs_detected();

#endif