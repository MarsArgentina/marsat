#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include "lightaprs.hpp"

//9xtend
#define xtendRX 1
#define xtendTx 3
#define xtendPWR 18

//payload1
#define payload1Rx 35
#define payload1Tx 33
//payload2
#define payload2Rx 21
#define payload2Tx 18


//lightaprs
#define aprsSDA 22
#define aprsSCL 23

//gpio
#define ballonGate 24
//TODO: definir pines de control de energía de payload

//Inicializaciones

Preferences memoria;

TinyGPSPlus gps;

//variables globales

int alturaParacaidas; //altura desde la cual desplegaremos el paracaídas en KM

int estadoVuelo = 0; //0: ascendiendo, 1:descendiendo, 2: aterrizado 

String serial0data, serial1data, serial2data;

int distanciaMaxima;
float distanciaVuelo;
float latitudInicial;
float longitudInicial;
float latitud, longitud;
int altura;

//temporizadores

uint32_t lastKAtime = 0;

//funciones propias
void getGps(float& latitud, float& longitud, int& altura);//por i2c le pide al APRS las coordenadas del gps
float getDistance(float flat1, float flon1, float flat2, float flon2);//calcula la distancia del gps respecto a las cooredenadas actuales
void checkUart(void);//reivsa los puertos uart 
String getDB(void);//pregunta al módulo de comunicaciones los dbs de recepcción

void setup() {
  //pines
  pinMode(ballonGate, OUTPUT);
  digitalWrite(ballonGate, HIGH);
  //comms
  Serial.begin(9600,SERIAL_8N1, xtendRX, xtendTx);
  Serial1.begin(9600, SERIAL_8N1, payload1Rx, payload1Tx);
  Serial2.begin(9600, SERIAL_8N1, payload2Rx, payload2Tx);
  Wire.begin(aprsSDA, aprsSCL);
  
  //busco en memoria los datos relevantes al control del vuelo
  memoria.begin("vuelo",false);
  alturaParacaidas = memoria.getInt("alturaParacaida",10);
  estadoVuelo = memoria.getInt("estadoVuelo",0);
  latitudInicial = memoria.getFloat("latitud",0);
  longitudInicial = memoria.getFloat("longitud",0);
  distanciaMaxima - memoria.getInt("maxdist",0);
  memoria.end();

  lightaprs_begin();
}


void loop() { 
  switch (estadoVuelo)
  {
  case 0:{
    checkUart();

    getGps(latitud, longitud, altura);
    distanciaVuelo = getDistance(latitud, longitud, latitudInicial, longitudInicial);
    if(distanciaVuelo > distanciaMaxima){
      Serial.write("Distancia excedida, terminando vuelo");
      //TODO: explotar globo
      estadoVuelo = 1;
      memoria.begin("vuelo",false);
      memoria.putInt("estadoVuelo",1);
      memoria.end();

      break;
    }

    break;
  }
  
  case 1: {
    //en este estado apagamos el payload

    break;
  }
  
  case 2: {

  }
  default:
    break;
  }
}

/**
 * @brief Función que verifica los puertos uart
 * Contesta el keep-alive de la estación terrena y también reenvia lo que tengan para reportar los payloads
 * 
 */
void checkUart(void){
  String respuesta;
  if( Serial.available() ){
    lastKAtime = millis();
    serial0data = Serial.readString();
    if( serial0data == "ka" ){//mensaje de keep-alive
      respuesta = getDB();
      respuesta = respuesta + "";//TODO: agregar coordenadas de gps
      Serial.write(respuesta.c_str());
    }
    else if( serial0data == "abort" ){
      //TODO: romper el globo
      //TODO: poner firmware en modo caída
      Serial.write("abortACK");
    }
    else if( serial0data == "comando" ){//TODO: definir prefijo de comando payload
      //TODO: reenviar comando a payload
    }
  }

  if( Serial1.available() ){
    serial1data = Serial.readString();
    Serial.write(serial1data.c_str());
  }

  if( Serial2.available() ){
    serial2data = Serial2.readString();
    Serial.write(serial2data.c_str());
  }

}

/**
 * @brief Función que le pide el módulo 9XTEND sus db's de recepción
 * 
 * @return String la respuesta del módulo 9xtend en dbs
 */
String getDB(void){

  static String respuestaXtend;
  Serial.write("+++\r");//entro en modo comandos
  while (Serial.available() == 0){//espera que llegue la respuesta
  }
  respuestaXtend = Serial.readString();
  Serial.write("ATDB\r");//pregunto los db
  while (Serial.available() == 0){//espera que llegue la respuesta
  }
  respuestaXtend = Serial.readString();
  Serial.write("ATCN\r");//salgo de modo comandos
  return respuestaXtend;//devuelvo la respuesta de los db

}


/**
 * @brief Obtener los datos de GPS desde el I2C. Obtiene como chars o strings y parsea a float
 * 
 * @param latitud puntero a donde guardar la latitud en punto flotante
 * @param longitud puntero a donde guardar la longitud en punto flotante
 * @param altura puntero a donde guardar la altura como int en KM
 */
void getGps(float& latitud, float& longitud, int& altura)
{

  //estandarizar comunicación con aprs
  Wire.requestFrom(8, 6);    // request 6 bytes from peripheral device #8
  while (Wire.available()) { // peripheral may send less than requested
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }

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
float getDistance(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc=0;
  float dist_calc2=0;
  float diflat=0;
  float diflon=0;

  // Calculations
  diflat  = radians(flat2-flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2)-(flon1));

  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
  dist_calc2 = cos(flat1);
  dist_calc2*=cos(flat2);
  dist_calc2*=sin(diflon/2.0);
  dist_calc2*=sin(diflon/2.0);
  dist_calc +=dist_calc2;

  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));
  
  dist_calc*=6371000.0; //Converting to meters

  return dist_calc;
}
