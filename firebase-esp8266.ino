
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h> 
#include <RFID.h>



// Insert your network credentials
#define WIFI_SSID "HANS-LAPTOP 9134"
#define WIFI_PASSWORD ";7Z8029u"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDzJQL11uqG5WkVuOldprykOyO7HomKhaY"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "attendance-system-for--iot-default-rtdb.firebaseio.com" 


RFID rfid(D8, D0);       //D8:pin of tag reader SDA. D0:pin of tag reader RST 
unsigned char str[MAX_LEN]; //MAX_LEN is 16: size of the array 
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line displ

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 10800; //(UTC+5:30)
NTPClient timeClient(ntpUDP, "pool.ntp.org");


FirebaseJson json;

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastMillis = 0;
const int red = D4;
const int green = D3;
String alertMsg;
String device_id="device11";
boolean checkIn = true;
int count = 0;
bool signupOK = false;

void setup(){
  Serial.begin(9600);


  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  lcd.init();                      // initialize the lcd 
  lcd.clear();
  lcd.backlight();

  SPI.begin();
  rfid.init();
  
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void checkAccess (String temp)    //Function to check if an identified tag is registered to allow access
{
    lcd.setCursor(1,0);   
    lcd.print("SCAN YOUR RFID");

    if(Firebase.RTDB.getInt(&fbdo, "/users/"+temp)){

        if (fbdo.intData() == 0)         //If firebaseData.intData() == checkIn
        {  
          alertMsg="CHECKING IN";
          lcd.setCursor(2,1);   
          lcd.print(alertMsg);
          delay(1000);

          json.add("time", String(timeClient.getFormattedTime()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",1);

          
          Firebase.RTDB.setInt(&fbdo, "users/"+temp,1);
          
          if (Firebase.RTDB.setJSON(&fbdo, "attendence/", &json)) {
            Serial.println(fbdo.dataPath() + fbdo.pushName()); 
            } else {
              Serial.println(fbdo.errorReason());
            }
        }
      else if (fbdo.intData() == 1)   //If the lock is open then close it
        { 
          alertMsg="CHECKING OUT";
          lcd.setCursor(2,1);   
          lcd.print(alertMsg);
          delay(1000);

          Firebase.RTDB.setInt(&fbdo, "users/"+temp,0);
          
          json.add("time", String(timeClient.getFormattedTime()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",0);
          if (Firebase.RTDB.setJSON(&fbdo, "attendence/", &json)) {
            Serial.println(fbdo.dataPath() + fbdo.pushName()); 
          } else {
            Serial.println(fbdo.errorReason());
          }
      }
 
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
}

void loop(){
  timeClient.update();
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
    Serial.println("Card found"); 
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
      Serial.print("The card's ID number is : "); 
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      } 
      Serial.println (temp);
      checkAccess (temp);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();

  lcd.setCursor(1,0);   
  lcd.print("SCAN YOUR RFID");
  lcd.setCursor(2,1);   
  lcd.print("GATE CLOSE");
  delay(500);
  lcd.clear();
}


