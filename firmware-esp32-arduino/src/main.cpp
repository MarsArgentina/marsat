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
#include "lightaprs.hpp"
#include <esp_task_wdt.h>

// 3 seconds WDT
#define WDT_TIMEOUT 5

#define PERIODO_TX_TMINUS 5000
#define PERIODO_TX_ASCENSO 60000
#define PERIODO_TX_DESCENSO 60000
#define PERIODO_TX_PARACAIDAS 60000
#define PERIODO_TX_RESCATE 60000

#define ALTURA_ASCENSO 1000
#define ALTURA_PARACAIDAS 10000

#define MAX_REINTENTOS_PAYLOADS 3

#define TIEMPO_MAX_SIN_KA 600000
#define ALTURA_MAX_VUELOS_COMERCIALES 12000

// 9xtend
#define XTEND_RX 1   // xtendRX
#define XTEND_TX 3   // xtendTX
#define XTEND_PWR 27 // xtendPWR

// payload1
#define PAYLOAD1_RX 35    // U1RXESP
#define PAYLOAD1_TX 33    // U1TXESP
#define PAYLOAD1_CHECK 13 // Q1
#define PAYLOAD1_ON 16    // uC_activation_FF1
#define PAYLOAD1_OFF 25

// payload2
#define PAYLOAD2_RX 21    // U2RXESP
#define PAYLOAD2_TX 18    // U2TXESP
#define PAYLOAD2_CHECK 34 // Q2
#define PAYLOAD2_ON 17    // uC_activation_FF2
#define PAYLOAD2_OFF 19

// lightaprs
#define LIGHTAPRS_SDA 14 // SDA
#define LIGHTAPRS_SCL 26 // SCL

// gpio
#define NICROM_PIN 23    // abortSig
#define PARACHUTE_PIN 22 // paracaidas

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

uint32_t distanciaMaxima;
float_t latitudInicial;
float_t longitudInicial;
float_t alturaAnterior;

lightaprs_t lightaprs;

uint8_t reintento_payload1, reintento_payload2;

// temporizadores

uint32_t lastKAtime = 0;
uint32_t lastmillis = 0;

// funciones propias
uint32_t getDistance(float flat1, float flon1, float flat2, float flon2); // calcula la distancia del gps respecto a las cooredenadas actuales
void checkUart(void);                                                     // reivsa los puertos uart
String getDB(void);                                                       // pregunta al módulo de comunicaciones los dbs de recepcción
void abortarVuelo(void);
void apagar_nicrom(void);
bool estadoPayload1(void);
bool estadoPayload2(void);
void guardar_estadoVuelo(void);
void apagarPayloads(void);
void mantenerPayloadsON(void);
void encenderPayload1(void);
void encenderPayload2(void);
void apagarPayload1(void);
void apagarPayload2(void);
void desactivar_paracaidas(void);
void activar_paracaidas(void);

void setup()
{
  // pines
  pinMode(XTEND_PWR, OUTPUT);
  digitalWrite(XTEND_PWR, HIGH);

  pinMode(PAYLOAD1_CHECK, INPUT);
  pinMode(PAYLOAD1_OFF, INPUT);
  pinMode(PAYLOAD1_ON, OUTPUT);
  digitalWrite(PAYLOAD1_ON, LOW);

  pinMode(PAYLOAD2_CHECK, INPUT);
  pinMode(PAYLOAD2_OFF, INPUT);
  pinMode(PAYLOAD2_ON, OUTPUT);
  digitalWrite(PAYLOAD2_ON, LOW);

  pinMode(NICROM_PIN, OUTPUT);
  digitalWrite(NICROM_PIN, LOW);
  pinMode(PARACHUTE_PIN, OUTPUT);
  digitalWrite(PARACHUTE_PIN, LOW);

  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch

  // comms
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, PAYLOAD1_RX, PAYLOAD1_TX);
  Serial2.begin(9600, SERIAL_8N1, PAYLOAD2_RX, PAYLOAD2_TX);
  Wire.begin(0x04, LIGHTAPRS_SDA, LIGHTAPRS_SCL, 100000);
  Serial.println("Iniciando");

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

    if ((millis() - lastmillis) > PERIODO_TX_TMINUS)
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
    if ((millis() - lastmillis) > PERIODO_TX_ASCENSO)
    {
      lastmillis = millis();
      Serial.printf("[ascenso] Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }

    if (getDistance(latitudInicial, longitudInicial, lightaprs.latitud, lightaprs.longitud) > distanciaMaxima)
    {
      Serial.println("Limite superado - Vuelo abortado");
      abortarVuelo();
    }

    if (lightaprs.altura < ALTURA_MAX_VUELOS_COMERCIALES && (millis() - lastKAtime) > TIEMPO_MAX_SIN_KA)
    {
      Serial.println("Comunicacion interrumpida - Vuelo abortado");
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
    if ((millis() - lastmillis) > PERIODO_TX_DESCENSO)
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

    apagarPayloads();

    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if ((millis() - lastmillis) > PERIODO_TX_PARACAIDAS)
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
    apagarPayloads();

    checkUart();

    if (new_data_available())
    {
      read_last_received(&lightaprs);
    }
    if ((millis() - lastmillis) > PERIODO_TX_RESCATE)
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
    lastKAtime = millis();
    serial0data = Serial.readString();

    if (serial0data == "estado")
    { // TODO: DB DEVUELVE UN STRING CON \R, ARREGLAR
      Serial.printf("[%d] dB: %s Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", estadoVuelo, getDB().c_str(), lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
      // Serial.printf("[%d] APRS: %d dB: %s Lat: %f, Lon: %f, Alt: %f, Temp_int: %f, Temp_ext: %f, Presion: %f\r\n", estadoVuelo, lightaprs_detected(), getDB().c_str(), lightaprs.latitud, lightaprs.longitud, lightaprs.altura, lightaprs.temp_int, lightaprs.temp_ext, lightaprs.presion);
    }
    else if (serial0data == "abort")
    {
      abortarVuelo();
      Serial.println("abortACK");
    }
    else if (serial0data == "abortOFF")
    {
      apagar_nicrom();
      Serial.println("abortOFFACK");
    }
    else if (serial0data == "paracaidasON")
    {
      activar_paracaidas();
      Serial.println("paracaidasONACK");
    }
    else if (serial0data == "paracaidasOFF")
    {
      desactivar_paracaidas();
      Serial.println("paracaidasOFFACK");
    }
    else if (serial0data == "payload1: ")
    {
      Serial1.print(serial0data.substring(10));
    }
    else if (serial0data == "payload2: ")
    {
      Serial2.print(serial0data.substring(10));
    }
    else if (serial0data == "payload1OFF")
    {
      apagarPayload1();
      Serial.println("payload1OFFACK");
    }
    else if (serial0data == "payload2OFF")
    {
      apagarPayload2();
      Serial.println("payload2OFFACK");
    }
    else if (serial0data == "payloadsOFF")
    {
      apagarPayloads();
      Serial.println("payloadsOFFACK");
    }
    else if (serial0data == "payload1ON")
    {
      encenderPayload1();
      Serial.println("payload1ONACK");
    }
    else if (serial0data == "payload2ON")
    {
      encenderPayload2();
      Serial.println("payload2ONACK");
    }
    else if (serial0data == "estadoPayloads")
    {
      Serial.printf("Payload1: %d, Payload2: %d\r\n", estadoPayload1(), estadoPayload2());
    }
    else if (serial0data == "distanciaMaxima: ") // TODO: cambiar interpetación de datos (strstr o stridx)
    {
      distanciaMaxima = serial0data.substring(17).toInt();
      Serial.printf("distanciaMaximaACK: %d\r\n", distanciaMaxima);
      memoria.putUInt("distanciaMaxima", distanciaMaxima);
    }
    else if (serial0data == "latitudInicial: ")
    {
      latitudInicial = serial0data.substring(16).toFloat();
      Serial.printf("latitudInicialACK: %f\r\n", latitudInicial);
    }
    else if (serial0data == "longitudInicial: ")
    {
      longitudInicial = serial0data.substring(17).toFloat();
      Serial.printf("longitudInicialACK: %f\r\n", longitudInicial);
    }
    else if (serial0data == "restart")
    {
      Serial.println("RestartACK");
      ESP.restart();
    }
    else
    {
      Serial.println("Comando no reconocido");
    }
  }

  if (Serial1.available())
  {
    serial1data = Serial1.readString();
    Serial.printf("Payload1: %s", serial1data.c_str());
  }

  if (Serial2.available())
  {
    serial2data = Serial2.readString();
    Serial.printf("Payload2: %s", serial2data.c_str());
  }
}

/**
 * @brief Función que le pide el módulo 9XTEND sus db's de recepción
 *
 * @return String la respuesta del módulo 9xtend en dbs
 */
String getDB(void)
{
  uint32_t lastmillis;

  String respuestaXtend;
  Serial.write("+++"); // entro en modo comandos

  lastmillis = millis();
  while (Serial.available() == 0 && (millis() - lastmillis) < 1000)
  { // espera que llegue la respuesta
  }
  Serial.readString();

  Serial.write("ATDB\r"); // pregunto los db
  lastmillis = millis();
  while (Serial.available() == 0 && (millis() - lastmillis) < 1000)
  { // espera que llegue la respuesta
  }
  respuestaXtend = Serial.readString();

  Serial.write("ATCN\r"); // salgo de modo comandos

  return respuestaXtend.substring(0, respuestaXtend.length() - 2); // devuelvo la respuesta de los db
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
uint32_t getDistance(float flat1, float flon1, float flat2, float flon2)
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

  dist_calc *= 6371000; // Converting to meters

  return (uint32_t)dist_calc;
}

void abortarVuelo(void)
{
  digitalWrite(NICROM_PIN, HIGH);
}

void apagar_nicrom(void)
{
  digitalWrite(NICROM_PIN, LOW);
}

void activar_paracaidas(void)
{
  digitalWrite(PARACHUTE_PIN, HIGH);
}

void desactivar_paracaidas(void)
{
  digitalWrite(PARACHUTE_PIN, LOW);
}

void encenderPayload1(void)
{
  if (!estadoPayload1())
  {
    digitalWrite(PAYLOAD1_ON, HIGH);
    delay(100);
    digitalWrite(PAYLOAD1_ON, LOW);
  }
}

void encenderPayload2(void)
{
  if (!estadoPayload2())
  {
    digitalWrite(PAYLOAD2_ON, HIGH);
    delay(100);
    digitalWrite(PAYLOAD2_ON, LOW);
  }
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

void apagarPayload1(void) // descomentar cuando se corrija hardware
{
  if (estadoPayload1())
  {
    pinMode(PAYLOAD1_OFF, OUTPUT);
    digitalWrite(PAYLOAD1_OFF, LOW);
    delay(100);
    pinMode(PAYLOAD1_OFF, INPUT);
  }
}

void apagarPayload2(void) // descomentar cuando se corrija hardware
{
  if (estadoPayload2())
  {
    pinMode(PAYLOAD2_OFF, OUTPUT);
    digitalWrite(PAYLOAD2_OFF, LOW);
    delay(100);
    pinMode(PAYLOAD2_OFF, INPUT);
  }
}

void apagarPayloads(void)
{
  apagarPayload1();
  apagarPayload2();
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