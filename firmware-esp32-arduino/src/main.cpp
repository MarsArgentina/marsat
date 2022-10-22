#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>

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

//variables globales

int alturaParacaidas; //altura desde la cual desplegaremos el paracaídas en KM

int estadoVuelo = 0; //0: ascendiendo, 1:descendiendo, 2: aterrizado 

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
  //cierro la memoria
  memoria.end();

}


void loop() { //Choose Serial1 or Serial2 as required
  String serial0data, serial1data, serial3data;

  if(Serial.available()){
    serial0data = Serial.readString();
    Serial1.write(serial0data.c_str());
  }

  // while (Serial2.available()) {
  //   Serial.print(char(Serial2.read()));
  // }
}