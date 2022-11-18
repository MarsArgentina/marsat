/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-11-17
 *
 * @copyright Copyright (c) 2022
 *
 *
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
// #include <TinyGPS++.h>
#include "lightaprs.hpp"
#include <esp_task_wdt.h>

//3 seconds WDT
#define WDT_TIMEOUT 3

#define PERIODO_TX_TMINUS 60000
#define PERIODO_TX_ASCENSO 60000
#define PERIODO_TX_DESCENSO 60000
#define PERIODO_TX_PARACAIDAS 60000
#define PERIODO_TX_RESCATE 60000

#define ALTURA_ASCENSO 1000
#define ALTURA_PARACAIDAS 10000

#define MAX_REINTENTOS_PAYLOADS 3

// 9xtend
#define XTEND_RX 1
#define XTEND_TX 3
#define XTEND_PWR 12

// payload1
#define PAYLOAD1_RX 35
#define PAYLOAD1_TX 33
#define PAYLOAD1_CHECK 13
#define PAYLOAD1_ON 16
#define PAYLOAD1_OFF 99 // TODO: definir pin

// payload2
#define PAYLOAD2_RX 21
#define PAYLOAD2_TX 18
#define PAYLOAD2_CHECK 34
#define PAYLOAD2_ON 17
#define PAYLOAD2_OFF 99 // TODO: definir pin

// lightaprs
#define LIGHTAPRS_SDA 26
#define LIGHTAPRS_SCL 14

// gpio
#define NICROM_PIN 23
#define PARACHUTE_PIN 22

// Inicializaciones

Preferences memoria;

// variables globales

// int alturaParacaidas; //altura desde la cual desplegaremos el paracaídas en KM

uint8_t estadoVuelo = 0;

enum estadosVuelo
{
  t_minus,
  ascenso,
  descenso,
  paracaidas,
  rescate
};

String serial0data, serial1data, serial2data;

float_t distanciaMaxima;
float_t latitudInicial;
float_t longitudInicial;
float_t alturaAnterior;

lightaprs_t lightaprs;

uint8_t reintento_payload1, reintento_payload2;

// temporizadores

uint32_t lastKAtime = 0;
uint32_t lastmillis = 0;

// funciones propias
float getDistance(float flat1, float flon1, float flat2, float flon2); // calcula la distancia del gps respecto a las cooredenadas actuales
void checkUart(void);                                                  // reivsa los puertos uart
String getDB(void);                                                    // pregunta al módulo de comunicaciones los dbs de recepcción
void abortarVuelo(void);
void apagar_nicrom(void);
bool estadoPayload1(void);
bool estadoPayload2(void);
void guardar_estadoVuelo(void);
void apagarPayloads(void);
void mantenerPayloadsON(void);

void setup()
{
  // pines
  pinMode(PAYLOAD2_OFF, INPUT);

  pinMode(XTEND_PWR, OUTPUT);
  digitalWrite(XTEND_PWR, HIGH);

  pinMode(PAYLOAD1_CHECK, INPUT);
  pinMode(PAYLOAD1_ON, OUTPUT);
  digitalWrite(PAYLOAD1_ON, LOW);

  pinMode(PAYLOAD2_CHECK, INPUT);
  pinMode(PAYLOAD2_ON, OUTPUT);
  digitalWrite(PAYLOAD2_ON, LOW);

  pinMode(NICROM_PIN, OUTPUT);
  digitalWrite(NICROM_PIN, LOW);
  pinMode(PARACHUTE_PIN, OUTPUT);
  digitalWrite(PARACHUTE_PIN, LOW);

  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

  // comms
  Serial.begin(9600, SERIAL_8N1, XTEND_RX, XTEND_TX);
  Serial1.begin(9600, SERIAL_8N1, PAYLOAD1_RX, PAYLOAD1_TX);
  Serial2.begin(9600, SERIAL_8N1, PAYLOAD2_RX, PAYLOAD2_TX);
  Wire.begin(LIGHTAPRS_SDA, LIGHTAPRS_SCL);
  

  // busco en memoria los datos relevantes al control del vuelo
  memoria.begin("vuelo", false);
  estadoVuelo = memoria.getUChar("estadoVuelo", 0);
  latitudInicial = memoria.getFloat("latitud", 0);
  longitudInicial = memoria.getFloat("longitud", 0);
  distanciaMaxima = memoria.getUInt("maxdist", 100000);
  memoria.end();

  // inicializo la comunicación con el módulo lightaprs
  lightaprs_begin();
}

void loop()
{
  switch (estadoVuelo)
  {
  case t_minus:
  {
    mantenerPayloadsON();
    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
      latitudInicial = lightaprs.latitud;
      longitudInicial = lightaprs.longitud;
    }

    if (millis() - lastmillis > PERIODO_TX_TMINUS)
    {
      lastmillis = millis();
      Serial.printf("[t-minus] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }

    if (lightaprs.altura > ALTURA_ASCENSO)
    {
      estadoVuelo = ascenso;
      guardar_estadoVuelo();
    }

    break;
  }

  case ascenso:
  {
    mantenerPayloadsON();
    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if (millis() - lastmillis > PERIODO_TX_ASCENSO)
    {
      lastmillis = millis();
      Serial.printf("[ascenso] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }

    if (getDistance(latitudInicial, longitudInicial, lightaprs.latitud, lightaprs.longitud) > distanciaMaxima)
    {
      Serial.println("Vuelo abortado");
      abortarVuelo();
    }

    if (lightaprs.altura <= alturaAnterior)
    {
      estadoVuelo = descenso;
      guardar_estadoVuelo();
    }
    else
    {
      alturaAnterior = lightaprs.altura;
    }

    break;
  }

  case descenso:
  {
    apagar_nicrom();
    mantenerPayloadsON();
    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if (millis() - lastmillis > PERIODO_TX_DESCENSO)
    {
      lastmillis = millis();
      Serial.printf("[descenso] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }
    if (lightaprs.altura >= alturaAnterior)
    {
      estadoVuelo = ascenso;
      guardar_estadoVuelo();
    }
    else
    {
      alturaAnterior = lightaprs.altura;
    }
    if (lightaprs.altura <= ALTURA_PARACAIDAS)
    {
      estadoVuelo = paracaidas;
      guardar_estadoVuelo();
    }
    break;
  }

  case paracaidas:
  {
    if (estadoPayload1() || estadoPayload2())
    {
      apagarPayloads();
    }
    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if (millis() - lastmillis > PERIODO_TX_PARACAIDAS)
    {
      lastmillis = millis();
      Serial.printf("[paracaidas] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }
    if (lightaprs.altura <= ALTURA_ASCENSO)
    {
      estadoVuelo = rescate;
      guardar_estadoVuelo();
    }
    break;
  }

  case rescate:
  {
    if (estadoPayload1() || estadoPayload2())
    {
      apagarPayloads();
    }
    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if (millis() - lastmillis > PERIODO_TX_RESCATE)
    {
      lastmillis = millis();
      Serial.printf("[rescate] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }
    break;
  }

  default:
    ESP.restart();
    break;
  }

  esp_task_wdt_reset();
}

/**
 * @brief Función que verifica los puertos uart
 * Contesta el keep-alive de la estación terrena y también reenvia lo que tengan para reportar los payloads
 *
 */
void checkUart(void)
{
  String respuesta;
  if (Serial.available())
  {
    lastKAtime = millis(); // TODO: debería abortar si no recibe ningún keep-alive en un tiempo determinado y si está en un rango de alturas peligroso para aviones
    serial0data = Serial.readString();

    if (serial0data == "ka")
    {
      Serial.printf("[%d] dB: %s Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", estadoVuelo, getDB().c_str(), lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }
    else if (serial0data == "abort")
    {
      abortarVuelo();
      Serial.write("abortACK");
    }
    else if (serial0data == "comando")
    { // TODO: definir prefijo de comando payload
      // TODO: reenviar comando a payload
    }
    // TODO: agregar comandos para encender/apagar payloads, para configurar coordenadas y distancia máxima
  }

  if (Serial1.available())
  {
    serial1data = Serial.readString();
    Serial.write(serial1data.c_str());
  }

  if (Serial2.available())
  {
    serial2data = Serial2.readString();
    Serial.write(serial2data.c_str());
  }
}

/**
 * @brief Función que le pide el módulo 9XTEND sus db's de recepción
 *
 * @return String la respuesta del módulo 9xtend en dbs
 */
String getDB(void)
{

  static String respuestaXtend;
  Serial.write("+++\r"); // entro en modo comandos
  while (Serial.available() == 0)
  { // espera que llegue la respuesta
  }
  respuestaXtend = Serial.readString();
  Serial.write("ATDB\r"); // pregunto los db
  while (Serial.available() == 0)
  { // espera que llegue la respuesta
  }
  respuestaXtend = Serial.readString();
  Serial.write("ATCN\r"); // salgo de modo comandos
  return respuestaXtend;  // devuelvo la respuesta de los db
}

/**
 * @brief Calcular la distancia entre dos puntos
 *
 * @param flat1 latitud
 * @param flon1 longitud
 * @param flat2 initiallatitud
 * @param flon2 initiallongitud
 * @return float Distancia en metros
 */
float getDistance(float flat1, float flon1, float flat2, float flon2)
{

  // Variables
  float dist_calc = 0;
  float dist_calc2 = 0;
  float diflat = 0;
  float diflon = 0;

  // Calculations
  diflat = radians(flat2 - flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2) - (flon1));

  dist_calc = (sin(diflat / 2.0) * sin(diflat / 2.0));
  dist_calc2 = cos(flat1);
  dist_calc2 *= cos(flat2);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc += dist_calc2;

  dist_calc = (2 * atan2(sqrt(dist_calc), sqrt(1.0 - dist_calc)));

  dist_calc *= 6371000.0; // Converting to meters

  return dist_calc;
}

void abortarVuelo(void)
{
  digitalWrite(NICROM_PIN, HIGH);
}

void apagar_nicrom(void)
{
  digitalWrite(NICROM_PIN, LOW);
}

void mantenerPayloadsON(void)
{
  if (!estadoPayload1() && reintento_payload1 <= MAX_REINTENTOS_PAYLOADS)
  {
    digitalWrite(PAYLOAD1_ON, HIGH);
    delay(100);
    digitalWrite(PAYLOAD1_ON, LOW);
    reintento_payload1++;
  }
  if (!estadoPayload2() && reintento_payload2 <= MAX_REINTENTOS_PAYLOADS)
  {
    digitalWrite(PAYLOAD2_ON, HIGH);
    delay(100);
    digitalWrite(PAYLOAD2_ON, LOW);
    reintento_payload2++;
  }
}

void apagarPayloads(void)
{
  if (estadoPayload1())
  {
    pinMode(PAYLOAD1_OFF, OUTPUT);
    digitalWrite(PAYLOAD1_OFF, LOW);
    delay(100);
    pinMode(PAYLOAD1_OFF, INPUT);
  }
  if (estadoPayload2())
  {
    pinMode(PAYLOAD2_OFF, OUTPUT);
    digitalWrite(PAYLOAD2_OFF, LOW);
    delay(100);
    pinMode(PAYLOAD1_OFF, INPUT);
  }
}

bool estadoPayload1(void)
{
  return digitalRead(PAYLOAD1_CHECK);
}

bool estadoPayload2(void)
{
  return digitalRead(PAYLOAD2_CHECK);
}

void guardar_estadoVuelo(void)
{
  memoria.begin("vuelo", false);
  memoria.putUChar("estadoVuelo", estadoVuelo);
  memoria.end();
}