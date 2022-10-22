#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <TinyGPS++.h>

//--------------------------------------------------------------
//enter your personal phone number to receive sms alerts.
//phone number must start with country code.
const String PHONE = "+923335376419";
//--------------------------------------------------------------
//GSM Module RX pin to Arduino 3
//GSM Module TX pin to Arduino 2
#define rxPin 2
#define txPin 3
SoftwareSerial SIM900(rxPin,txPin);
//--------------------------------------------------------------
//GPS Module RX pin to Arduino 9
//GPS Module TX pin to Arduino 8
AltSoftSerial neogps;
TinyGPSPlus gps;
//--------------------------------------------------------------
#define BUZZER 4

// Alarm
int buzzer_timer = 0;
bool alarm = false;
boolean send_alert_once = true;

//----------------------------------
// keep track of time, after how much time the messages are sent

// Tracks the time since last event fired
boolean multiple_sms = false;
unsigned long previousMillis=0;
unsigned long int previoussecs = 0; 
unsigned long int currentsecs = 0; 
 unsigned long currentMillis = 0; 
 int secs = 0; 
 int pmints = 0; 
 int mints = 0; // current mints
 int interval= 1 ; // updated every 1 second

 
//--------------------------------------------------------------
// Size of the geo fence (in meters)
const float maxDistance = 20;

//--------------------------------------------------------------
float initialLatitude = 33.094691;
float initialLongitude = 71.166038;

float latitude, longitude;

//--------------------------------------------------------------


void getGps(float& latitude, float& longitude);


/*****************************************************************************************
 * setup() function
 *****************************************************************************************/
void setup()
{
  //--------------------------------------------------------------
  //Serial.println("Arduino serial initialize");
  Serial.begin(9600);
  //--------------------------------------------------------------
  //Serial.println("SIM800L serial initialize");
  SIM900.begin(9600);
  //--------------------------------------------------------------
  //Serial.println("NEO6M serial initialize");
  neogps.begin(9600);
  //--------------------------------------------------------------
  pinMode(BUZZER, OUTPUT);
  //--------------------------------------------------------------
  SIM900.println("AT"); //Check GSM Module
  delay(1000);
  SIM900.println("ATE1"); //Echo ON
  delay(1000);
  SIM900.println("AT+CPIN?"); //Check SIM ready
  delay(1000);
  SIM900.println("AT+CMGF=1"); //SMS text mode
  delay(1000);
  SIM900.println("AT+CNMI=1,1,0,0,0"); /// Decides how newly arrived SMS should be handled
  delay(1000);
  //AT +CNMI = 2,1,0,0,0 - AT +CNMI = 2,2,0,0,0 (both are same)
  //--------------------------------------------------------------
  delay(20000);
  buzzer_timer = millis();
}





/*****************************************************************************************
 * loop() function
 *****************************************************************************************/
void loop()
{
  //--------------------------------------------------------------
  getGps(latitude, longitude);
  //--------------------------------------------------------------
  float distance = getDistance(latitude, longitude, initialLatitude, initialLongitude);
  //--------------------------------------------------------------
  Serial.print("Latitude= "); Serial.println(latitude, 6);
  Serial.print("Lngitude= "); Serial.println(longitude, 6);
  Serial.print("initialLatitude= "); Serial.println(initialLatitude, 6);
  Serial.print("initialLngitude= "); Serial.println(initialLongitude, 6);
  Serial.print("current Distance= "); Serial.println(distance);
  //--------------------------------------------------------------
  // Set alarm on?
  if(distance > maxDistance) {
    multiple_sms = true;
    //------------------------------------------
    if(send_alert_once == true){
      digitalWrite(BUZZER, HIGH);
      sendAlert();
      alarm = true;
      send_alert_once = false;
      buzzer_timer = millis();
      
    }
    //------------------------------------------
  }
  else{
    send_alert_once = true;
    multiple_sms = false;
  }
  //--------------------------------------------------------------

  // Handle alarm
  if (alarm == true) {
    if (millis() - buzzer_timer > 5000) {
      digitalWrite(BUZZER, LOW);
      alarm = false;
      buzzer_timer = 0;
      
    }
  }

  if ( multiple_sms = true)
  {
       currentMillis = millis();
       currentsecs = currentMillis / 1000; 
       if ((unsigned long)(currentsecs - previoussecs) >= interval) {
       secs = secs + 1; 

       if ( secs >= 20)
       {
        sendAlert();
        multiple_sms = false;
        secs = 0;
       }
    }
  }
  //--------------------------------------------------------------  
  while(SIM900.available()){
    Serial.println(SIM900.readString());
  }
  //--------------------------------------------------------------
  while(Serial.available())  {
    SIM900.println(Serial.readString());
  }
  //--------------------------------------------------------------


}



/*****************************************************************************************
* getDistance() function
*****************************************************************************************/

// Calculate distance between two points
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


/*****************************************************************************************
 * getGps() Function
*****************************************************************************************/
void getGps(float& latitude, float& longitude)
{
  // Can take up to 60 seconds
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }
  }
  
  if (newData) //If newData is true
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    newData = false;
  }
  else {
    Serial.println("No GPS data is available");
    latitude = 0;
    longitude = 0;
  }
}




/*****************************************************************************************
* sendAlert() function
*****************************************************************************************/
void sendAlert()
{
  //return;
  String sms_data;
  sms_data = "Alert! The Car is outside the fence.\r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += String(latitude) + "," + String(longitude);

  //return;
  SIM900.print("AT+CMGF=1\r");
  delay(1000);
  SIM900.print("AT+CMGS=\""+PHONE+"\"\r");
  delay(1000);
  SIM900.print(sms_data);
  delay(100);
  SIM900.write(0x1A); //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
  delay(1000);
  Serial.println("SMS Sent Successfully.");
  
}