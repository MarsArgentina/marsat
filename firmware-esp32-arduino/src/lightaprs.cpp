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

bool new_data = false, detectado = false;

void receive_event(int howMany)
{
    int16_t ret;
    byte codigo = Serial2.read();
    Serial.print("Recibidos ");
    Serial.print(howMany);
    Serial.print(" bytes del LightAPRS. Codigo: ");
    Serial.println(codigo);

    if (howMany >= 13)
    {
        /* code */

        if (codigo == 0x01)
        {
            for (int i = 0; i < 4; i++)
            {
                latitud.bytes[i] = Serial2.read();
            }
            for (int i = 0; i < 4; i++)
            {
                longitud.bytes[i] = Serial2.read();
            }
            for (int i = 0; i < 4; i++)
            {
                altura.bytes[i] = Serial2.read();
            }
            new_data = true;
        }
        else if (codigo == 0x02)
        {
            for (int i = 0; i < 4; i++)
            {
                temp_int.bytes[i] = Serial2.read();
            }
            for (int i = 0; i < 4; i++)
            {
                temp_ext.bytes[i] = Serial2.read();
            }
            for (int i = 0; i < 4; i++)
            {
                presion.bytes[i] = Serial2.read();
            }
            new_data = true;
        }
        while (Serial2.available())
        {
            Serial2.read();
        }
    }
}

void read_last_received(lightaprs_t *lightaprs)
{
    lightaprs->altura = altura.number;
    lightaprs->latitud = latitud.number;
    lightaprs->longitud = longitud.number;
    lightaprs->presion = presion.entero;
    lightaprs->temp_ext = temp_ext.number;
    lightaprs->temp_int = temp_int.number;
    new_data = false;
}

bool new_data_available()
{
    return new_data;
}

// bool lightaprs_detected()
// {
//     return detectado;
// }