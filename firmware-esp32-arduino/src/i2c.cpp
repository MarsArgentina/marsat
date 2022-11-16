/**
 * @file i2c.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-11-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "i2c.hpp"

floatunion_t latt, lon, alt, temp, pres;

void receiveEvent(int howMany)
{
    int16_t ret;
    byte codigo = Wire.read();
    Serial.print("Recibidos ");
    Serial.print(howMany);
    Serial.print(" bytes, codigo: ");
    Serial.println(codigo);

    if (codigo == 0x01)
    {
        for (int i = 0; i < 4; i++)
        {
            latt.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            lon.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            alt.bytes[i] = Wire.read();
        }
    }
    else if (codigo == 0x02)
    {
        for (int i = 0; i < 4; i++)
        {
            temp.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            pres.bytes[i] = Wire.read();
        }
    }

    else
    {
        while (Wire.available())
        {
            Wire.read();
        }
    }
}

void setupI2C()
{
    Wire.begin(0x04);             // join i2c bus with address #4
    Wire.onReceive(receiveEvent); // register event
}
