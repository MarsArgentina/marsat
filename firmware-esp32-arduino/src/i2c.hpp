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

#ifndef I2C_HPP
#define I2C_HPP

#include <Arduino.h>
#include <Wire.h>

typedef union
{
  float number;
  uint8_t bytes[4];
} floatunion_t;

void setupI2C();



#endif