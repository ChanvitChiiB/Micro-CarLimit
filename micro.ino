#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include "time.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define FIREBASE_HOST "https://micro-6910f-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "8KUYhtIMQogsCIE0sgfRmpthIQ0oE62yHItGxo5k"
#define API_KEY "AIzaSyCUoC_w3nwSFJurqSp-hWDadQQ2HMHtBtQ"
#define FIREBASE_PROJECT_ID "micro-6910f"
#define ntp_Server "pool.ntp.org"
#define ssid "ChiiB"
#define password "0653016872B"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const long gmtOffset_sec = 7*3600;
const int daylightOffset_sec = 0;

int pinIN = 34;
int pinOUT = 35;

int statusIN = -1;
int statusOUT = -1;
int current = 0;
int maximum = 20;
int tolerance = 10;
long inTime;
long outTime;
char _tableValues[30];
String documentPath = "Micro/car-limit";
String current_input;

void getTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    getTime();
  }
  strftime(_tableValues, sizeof(_tableValues), "/%d_%m_%Y/time-%H:%M:%S", &timeinfo);
  Serial.println(_tableValues);
}

void checkIN(){
  if (statusIN != digitalRead(pinIN)) {
    statusIN = digitalRead(pinIN);
    if(digitalRead(pinIN) == 0 && digitalRead(pinOUT) == 1){
      inTime = millis();

      while ((millis() - inTime) <= 5000) {
        if(digitalRead(pinIN) == 1 && digitalRead(pinOUT) == 0){
          outTime = millis();
          sender();
          break;
        }
      }

    }
  }
}

void checkOUT(){
  if (statusOUT != digitalRead(pinOUT)) {
    statusOUT = digitalRead(pinOUT);
    if(digitalRead(pinOUT) == 0 && digitalRead(pinIN) == 1){
      outTime = millis();
      
      while ((millis() - outTime) <= 5000) {
        if(digitalRead(pinOUT) == 1&& digitalRead(pinIN) == 0){
          inTime = millis();
          sender();
          break;
        }
      }
      
    }
  }
}

void connectWifi(){
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected!");
}

void sender(){
  if (inTime - outTime < tolerance) {
    getTime();
    current++;
    Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, _tableValues, F("IN")) ? "in ok" : fbdo.errorReason().c_str());
    FirebaseJson content;
    content.set("fields/current/doubleValue", String(current).c_str());
    content.set("fields/maximum/doubleValue", String(maximum).c_str());
  
    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(),"maximum,current")){
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        return;
    }else{
        Serial.println(fbdo.errorReason());
    }
  }
  
  if (outTime - inTime < tolerance) {
    getTime();
    current--;
    Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, _tableValues, F("OUT")) ? "out ok" : fbdo.errorReason().c_str());
    
    if(current < 0)
      current = 0;
    
    FirebaseJson content;
    content.set("fields/current/doubleValue", String(current).c_str());
    content.set("fields/maximum/doubleValue", String(maximum).c_str());
  
    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(),"maximum,current")){
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        return;
    }else{
        Serial.println(fbdo.errorReason());
    }
  }

}

void setup() {
  Serial.begin(115200);

  pinMode(pinIN, INPUT);
  pinMode(pinOUT, INPUT);

  connectWifi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntp_Server);
  config.api_key = API_KEY;
  config.database_url = FIREBASE_HOST;
  Firebase.reconnectWiFi(true);
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), current_input.c_str())){
    current = atoi(current_input.c_str());
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  }
  else
    Serial.println(fbdo.errorReason());
  
}

void loop() {

  checkIN();
  checkOUT();
  
}
