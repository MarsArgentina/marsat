#include <Arduino.h>
#include <Wire.h>
/*
 * There are three serial ports on the ESP known as U0UXD, U1UXD and U2UXD.
 * 
 * U0UXD is used to communicate with the ESP32 for programming and during reset/boot.
 * U1UXD is unused and can be used for your projects. Some boards use this port for SPI Flash access though
 * U2UXD is unused and can be used for your projects.
 * 
*/

//9xtend
#define xtendRX 16
#define xtendTx 17
#define xtendPWR 18

//payload
#define payloadRx 20
#define payloadTx 21

//lightaprs
#define aprsSDA 22
#define aprsSCL 23

//gpio
#define ballonGate 24


void setup() {

  Serial.begin(115200);

  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial1.begin(9600, SERIAL_8N1, xtendRX, xtendTx);
  Serial2.begin(9600, SERIAL_8N1, payloadRx, payloadTx);

  Wire.begin(aprsSDA, aprsSCL);

  pinMode(ballonGate, OUTPUT);

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