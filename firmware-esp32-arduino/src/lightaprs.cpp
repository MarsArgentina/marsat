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

#include "lightaprs.hpp"

floatunion_t latitud, longitud, altura, temp_int, temp_ext, presion;

void receive_event(int howMany)
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
            latitud.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            longitud.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            altura.bytes[i] = Wire.read();
        }
    }
    else if (codigo == 0x02)
    {
        for (int i = 0; i < 4; i++)
        {
            temp_int.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            temp_ext.bytes[i] = Wire.read();
        }
        for (int i = 0; i < 4; i++)
        {
            presion.bytes[i] = Wire.read();
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

void lightaprs_begin()
{
    Wire.begin(0x04);             // join i2c bus with address #4
    Wire.onReceive(receive_event); // register event
}


void read_last_received(lightaprs_t *lightaprs)
{
    lightaprs->altura = altura.number;
    lightaprs->latitud = latitud.number;
    lightaprs->longitud = longitud.number;
    lightaprs->presion = presion.number;
    lightaprs->temp_ext = temp_ext.number;
    lightaprs->temp_int = temp_int.number;
}