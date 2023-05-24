/*
   Rui Santos
   Complete project details at our blog.
   - ESP32: https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
   - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   Based in the RTDB Basic Example by Firebase-ESP-Client library by mobizt
https://github.com/mobizt/Firebase-ESP-Client/blob/main/examples/RTDB/Basic/Basic.ino
 */

//#include <Arduino.h>
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

// Insert your network credentials
//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
/* String uid; */

unsigned long sendDataPrevMillis = 0;
int count = 0;
uint8_t number = 0;
uint8_t recieveNumber = 0;
uint8_t targetMoisture = 0;
uint8_t pumpTrigger = 0;
uint8_t pumpTime = 0;
bool signupOK = true;
unsigned long sendInterval = 100;
unsigned long historyInterval = 60000;
unsigned long historyTimer = 0;
unsigned long getTargetMoistTimer = 0;
unsigned long getTargetMoistInterval = 10000;
unsigned long setMoistTimer = 0;
unsigned long setMoistInterval = 7303;
bool pumpState = false;
bool automaticWateringBool = false;
uint8_t automaticWateringInt = 0;
bool uartFound = false;

//previous values
uint8_t moisture_old;
uint8_t waterLevel_old;
uint8_t pumpState_old = 0;

uint8_t txBuffer[6];

const int BUF_SIZE = 6;
uint8_t dataToFirebase[BUF_SIZE];
const int uartRxBufferSize = 30;
uint8_t uartIncomingBuffer[uartRxBufferSize];

bool firstPass = true;
bool reZeroPump = true;

void uartRead();

void setup() {
  dataToFirebase[1] = 255;
  dataToFirebase[2] = 255;
  dataToFirebase[5] = 0;
  txBuffer[0] = 254;
  txBuffer[5] = 0;

  Serial.setTimeout(200);
  Serial.begin(115200);
  //Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  //Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(".");
    delay(300);
  }
  fbdo.setResponseSize(4096);
  /* fbdo.setBSSLBufferSize(2048, 2048); */
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  /* config.signer.test_mode = true; */

  /* Sign up */
  /* if (Firebase.signUp(&config, &auth, "", "")) { */
  /* //Serial.println("ok"); */
  /*   signupOK = true; */
  /* } else { */
  /* //Serial.printf("%s\n", config.signer.signupError.message.c_str()); */
  /* } */

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h
                                                       // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  historyTimer = millis();
  getTargetMoistTimer = millis();
  setMoistTimer = millis();
}

void loop() {
  if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
  }
  //txBuffer[1] = 255;
  uartRead();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > sendInterval || sendDataPrevMillis == 0)) {



    /* if ((dataToFirebase[4] & 0x1) == 0x0) { */


    //recieveNumber = Serial.read();

    /* if (dataToFirebase[0] == 254 && dataToFirebase[6] == 254) { */
    /* if (uartFound) { */
    /* if(millis() - setMoistTimer > setMoistInterval) */
    /* if (dataToFirebase[5] == 0) { */
    /*     pumpState = false; */
    /*     //pumpState2 = 0; */
    /* } else if (dataToFirebase[5] == 1) { */
    /*     pumpState = true; */
    /*     //pumpState2 = 1; */
    /* } */
    //--------------------------------set manual watering to zero

    if (dataToFirebase[5] != pumpState_old || firstPass) {
      /* Firebase.RTDB.setBool(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pumpState", pumpState); */
      Firebase.RTDB.setBoolAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pumpState", dataToFirebase[5]);
      pumpState_old = dataToFirebase[5];
    }

    if (millis() - setMoistTimer > setMoistInterval) {
      if (dataToFirebase[1] != moisture_old || firstPass) {
        Firebase.RTDB.setIntAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/currentMoisture", dataToFirebase[1]);
        moisture_old = dataToFirebase[1];
      }

      if (dataToFirebase[2] != waterLevel_old || firstPass) {
        Firebase.RTDB.setIntAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/waterLevel", dataToFirebase[2]);
        waterLevel_old = dataToFirebase[2];
      }
      setMoistTimer = millis();
    }

    if (millis() - historyTimer > historyInterval) {
      historyTimer = millis();

      Firebase.RTDB.pushInt(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureHistory", 1);
      String pushname = fbdo.pushName();

      Firebase.RTDB.setIntAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureHistory/" + pushname + "/moisture", dataToFirebase[1]);
      Firebase.RTDB.setTimestampAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureHistory/" + pushname + "/timeStamp");
      /* Firebase.RTDB.setInt(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureHistory", dataToFirebase[1]); */
      //String history= String("/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureHistory/" + fbdo.pushName() + "/");
      //strcat(history, &fbdo.pushName());
      //Firebase.RTDB.pushInt(&fbdo, history, dataToFirebase[1]);
    }
    /* } */
    uartRead();
    Serial.write(txBuffer, 6);

    if (dataToFirebase[5] != pumpState_old || firstPass) {
      /* Firebase.RTDB.setBool(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pumpState", pumpState); */
      Firebase.RTDB.setBoolAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pumpState", dataToFirebase[5]);
      pumpState_old = dataToFirebase[5];
    }

    //--------------------------------
    firstPass = false;
    /* } else { */
    /*     //sanitize buffer */
    /*     for (int i = 0; i < Serial.available(); i++) { */
    /*         Serial.read(); */
    /*     } */
    /* } */

    //if (Firebase.RTDB.getInt(&fbdo, "/motor")) {
    if (millis() - getTargetMoistTimer > getTargetMoistInterval) {
      if (Firebase.RTDB.getInt(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/moistureLevel")) {
        if (fbdo.dataType() == "int") {
          targetMoisture = fbdo.intData();
          txBuffer[1] = fbdo.intData();
        }
      }
      if (Firebase.RTDB.getInt(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pumpTime")) {
        if (fbdo.dataType() == "int") {
          pumpTime = fbdo.intData();
          txBuffer[2] = fbdo.intData();
        }
      }
    }
    uartRead();
    if (reZeroPump && dataToFirebase[5] == 0) {
      Firebase.RTDB.setIntAsync(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pump", 0);
      reZeroPump = false;
    }
    if (dataToFirebase[5] == 0) {
      txBuffer[3] = 0;
    }
    if (Firebase.RTDB.getInt(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/pump")) {
      if (fbdo.dataType() == "int") {
        pumpTrigger = fbdo.intData();
        if (txBuffer[3] == 0)
          txBuffer[3] = pumpTrigger;
        if (pumpTrigger == 1) {
          reZeroPump = true;
          //pumpTrigger = 0;
        }
      }
    }
    if (Firebase.RTDB.getBool(&fbdo, "/TvJf12v5TjekpbcQC87jCr0K3Wj1/devices/device1/autoWateringState")) {
      /* if (fbdo.dataType() == "bool") { */
      automaticWateringBool = fbdo.boolData();
      if (automaticWateringBool) {
        automaticWateringInt = 1;
      } else {
        automaticWateringInt = 0;
      }
      txBuffer[4] = automaticWateringInt;
      /* } */
    }

    //txBuffer[0] = 5;
    //txBuffer[1] = 6;
    //txBuffer[2] = 7;
    /* if (txBuffer[1] != 255) { */
    Serial.write(txBuffer, 6);
    //set pumpTrigger to 0
    /* } */
    sendDataPrevMillis = millis();
  }
}



void uartRead() {
  if (Serial.available() > 6) {
    //Serial.readBytes(uartIncomingBuffer, Serial.available());
    Serial.readBytesUntil(127, uartIncomingBuffer, Serial.available());
  } else {
    //uartFound = false;
    return;
  }
  uint8_t startBlock = 0;
  /* uint8_t endBlock = 255; */
  bool foundStart = false;
  bool foundEnd = false;
  for (int i = 0; i < uartRxBufferSize; i++) {
    if (uartIncomingBuffer[i] == 254) {
      startBlock = i;
      foundStart = true;
      uartFound = true;
      break;
    }
  }
  /* if(foundStart) */
  /* { */
  /*     for(int i = startBlock + 1; i < uartRxBufferSize; i++) */
  /*     { */
  /*         if(i == startBlock + 6 && uartIncomingBuffer[i] == 127 && foundStart) */
  /*         { */
  /*             endBlock = i; */
  /*             //foundEnd = true; */
  /*             uartFound = true; */
  /*             break; */
  /*         }else{ */
  /*             uartFound = false; */
  /*         } */

  /*     } */
  /* } */

  //if(foundStart && foundEnd)
  if (foundStart) {
    for (int i = 0; i < BUF_SIZE; i++) {
      dataToFirebase[i] = uartIncomingBuffer[startBlock + i];
    }
  }
  char dummy;
  while(Serial.available() > 0)
  {
    dummy = Serial.read();
  }

  return;
}
