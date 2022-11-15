#include <Arduino.h>
#include "HardwareSerial.h"

HardwareSerial Serial1;
#define xtendRX 16
#define xtendTX 17


void setup() {
  Serial1.begin(9600,SERIAL_8N1,xtendRX,xtendTX,false); //puerto serie de HW donde conectamos el módulo XTEND

  Serial.begin(9600); //puerto serie por defecto de arduino, en el ESP32 usar el 0 que es donde hacemos el debbuging
  
}
void loop() {


  if (Serial.available()) {      // Si enviamos algo por terminal


    Serial1.write(Serial.read());   // lo manda a Serial 1


  }


  if (Serial1.available()) {     // Si algo viene del Serial1


    Serial.write(Serial1.read());   // lo manda al serial 0


  }

}