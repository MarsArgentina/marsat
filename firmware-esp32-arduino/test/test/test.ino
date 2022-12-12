#include <SoftwareSerial.h>
#define ESP32_RX 6
#define ESP32_TX 5

typedef union
{
  int32_t entero;
  float flotante;
  uint8_t bytes[4];
} floatunion_t;

floatunion_t latt, lon, alt, temp_int, temp_ext, pres, vbat;


SoftwareSerial ESP_Serial(ESP32_RX, ESP32_TX); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  ESP_Serial.begin(9600);
  initial_msg_i2c();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {
    int a = Serial.read();
    switch (a) {
      case 0: tminus();
        break;
      case 1: ascenso();
        break;
      case 2: descenso();
        break;
      case 3: paracaidas();
        break;
      case 4: rescate();
        break;
      case 5: fuera();
        break;
      default:
        while (Serial.available()) {
          Serial.read();
        };
        break;
    }
  }
}

void tminus()
{
  
}

void ascenso()
{
  
}

void descenso()
{
  
}

void paracaidas()
{
  
}

void rescate()
{
  
}

void fuera()
{
  
}

void initial_msg_i2c()
{
  latt.flotante = -11.111111;
  lon.flotante = -22.222222;
  alt.flotante = 333.33;
  temp_int.flotante = 44.44;
  temp_ext.flotante = -55.55;
  pres.entero = 777;
  send_coord_i2c();
  delay(1000);
}

void send_coord_i2c()
{
  ESP_Serial.write(0x01);
  for (int i = 0; i < 4; i++)
  {
    ESP_Serial.write(latt.bytes[i]);
  }
  for (int i = 0; i < 4; i++)
  {
    ESP_Serial.write(lon.bytes[i]);
  }
  for (int i = 0; i < 4; i++)
  {
    ESP_Serial.write(alt.bytes[i]);
  }
}
