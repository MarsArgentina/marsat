#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <TinyGPS++.h>

/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 * 
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 * 
*/

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

//Inicializaciones

Preferences memoria;

TinyGPSPlus gps;

//variables globales

int alturaParacaidas; //altura desde la cual desplegaremos el paracaídas en KM

int estadoVuelo = 0; //0: ascendiendo, 1:descendiendo, 2: aterrizado 

//--------------------------------------------------------------
// Size of the geo fence (in meters)
const float maxDistance = 20;

//--------------------------------------------------------------
float latitudInicial;
float longitudInicial;

float latitud, longitud;
int altura;

void getGps(float& latitud, float& longitud, int& altura);
float getDistance(float flat1, float flon1, float flat2, float flon2);


void setup() {

  Serial.begin(9600,SERIAL_8N1, xtendRX, xtendTx);

  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial1.begin(9600, SERIAL_8N1, payload1Rx, payload1Tx);
  Serial2.begin(9600, SERIAL_8N1, payload2Rx, payload2Tx);

  Wire.begin(aprsSDA, aprsSCL);

  pinMode(ballonGate, OUTPUT);

  //busco en memoria los datos relevantes al control del vuelo
  memoria.begin("vuelo",false);

  alturaParacaidas = memoria.getInt("alturaParacaida",10);
  estadoVuelo = memoria.getInt("estadoVuelo",0);
  latitudInicial = memoria.getFloat("latitud",0);
  longitudInicial = memoria.getFloat("longitud",0);
  //cierro la memoria
  memoria.end();

}


void loop() { 

  String serial0data, serial1data, serial3data;

  if(Serial.available()){
    serial0data = Serial.readString();
    Serial1.write(serial0data.c_str());
  }

  // while (Serial2.available()) {
  //   Serial.print(char(Serial2.read()));
  // }

  //--------------------------------------------------------------
  getGps(latitud, longitud, altura);
  //--------------------------------------------------------------
  float distance = getDistance(latitud, longitud, initiallatitud, initiallongitud);
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
