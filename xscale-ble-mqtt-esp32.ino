#include "BLEDevice.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

  char* ssid = "***";
  char* password = "***";

  int sex=1; // male = 1; female = 0
  float height=183;
  boolean fitnessBodyType=true;
  int age=38;

  char* mac="0c:b2:b7:aa:aa:aa";

 char* mqttServer = "172.16.2.1";
 int mqttPort = 1883;
 char* mqttUser = "";
 char* mqttPassword = "";
// The remote service we wish to connect to.
 BLEUUID serviceUUID("0000ffe0-0000-1000-8000-00805f9b34fb");
 BLEUUID serviceUUID1("0000ffe5-0000-1000-8000-00805f9b34fb");

 BLEUUID    charUUID("0000ffe4-0000-1000-8000-00805f9b34fb");
 BLEUUID    charUUID1("0000ffe9-0000-1000-8000-00805f9b34fb");

WiFiClient espClient;
PubSubClient client(espClient);

 BLEAddress *pServerAddress;
 boolean doConnect = false;
 boolean connected = false;
 boolean first = true;
 BLERemoteCharacteristic* pRemoteCharacteristic;
 BLERemoteCharacteristic* pRemoteCharacteristic1;

void connectMQTT(){
    while (!client.connected()) {
    Serial.println(F("Connecting to MQTT..."));
 
    if (client.connect("XScale", mqttUser, mqttPassword )) {
 
      Serial.println(F("connected"));  
 
    } else {
 
      Serial.print(F("failed with state "));
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if(length<20) return;
    Serial.print(F("Data "));
    char buffer[3];
    int i;
    for (i = 0; i < length; i++) {
      sprintf(buffer, "%02X", pData[i]);
      buffer[2]=0x00;
      Serial.print(buffer);
    }
    Serial.println();

    processData(pData);
    
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println(F("onDisconnect"));
    ESP.restart();
  }
};

bool connectToServer(BLEAddress pAddress) {
    Serial.print(F("Forming a connection to "));
    Serial.println(pAddress.toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(F(" - Created client"));

    pClient->setClientCallbacks(new MyClientCallback());

    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    // Connect to the remove BLE Server.
    pClient->connect(pAddress);
    Serial.println(F(" - Connected to server"));

    esp_task_wdt_delete(NULL);
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print(F("Failed to find our service UUID: "));
      Serial.println(serviceUUID.toString().c_str());
      connected = false;
      return false;
    }
    Serial.println(F(" - Found our service"));
    
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService1 = pClient->getService(serviceUUID1);
    if (pRemoteService1 == nullptr) {
      Serial.print(F("Failed to find our service UUID: "));
      Serial.println(serviceUUID1.toString().c_str());
      connected = false;
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print(F("Failed to find our characteristic UUID: "));
      Serial.println(charUUID.toString().c_str());
      connected = false;
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic1 = pRemoteService1->getCharacteristic(charUUID1);
    if (pRemoteCharacteristic1 == nullptr) {
      Serial.print(F("Failed to find our characteristic UUID: "));
      Serial.println(charUUID1.toString().c_str());
      connected = false;
      return false;
    }
    Serial.println(F(" - Found our characteristic"));

    pRemoteCharacteristic->registerForNotify(notifyCallback);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting Arduino BLE Client application..."));
  client.setServer(mqttServer, mqttPort);
  BLEDevice::init("");
  
  pServerAddress = new BLEAddress(mac);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(F("Connection Failed! Rebooting..."));
    delay(5000);
    ESP.restart();
  }


  connectMQTT();

  ArduinoOTA.setHostname("XScale");

  // No authentication by default
  ArduinoOTA.setPassword("bpetar");


  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      Serial.println(F("End Failed"));
    }
  });
  ArduinoOTA.begin();

} // End of setup.


// This is the Arduino main loop function.
void loop() {  
  connectMQTT();
  ArduinoOTA.handle();
  
  if (connected == false) {
    if (connectToServer(*pServerAddress)) {
      Serial.println(F("We are now connected to the BLE Server."));
      connected = true;
    } else {
      Serial.println(F("We have failed to connect to the server; there is nothin more we will do."));
    }
  }

  
  if (connected && first) {
    byte byteArray[]={0x0d,0x12,0x10,0x03,0x00,0x16,0xd2,0x98,0xb4,0x01,0x27,0x00,0x00,0x00,0x00,0x01,0x00,0xce};
    pRemoteCharacteristic1->writeValue(byteArray, sizeof(byteArray));
    first=false;
  }


//    std::string value = pRemoteCharacteristic->readValue();
//    Serial.println(value.c_str());

  
  delay(1000); // Delay a second between loops.
} // End of loop

void sendMQTT(char* topic,float val){
  String temp_str;
  temp_str = String(val); 
  char temp[temp_str.length() + 1];
  temp_str.toCharArray(temp, temp_str.length()+1);
  client.publish(topic, temp);
}

void processData(uint8_t* data){
  char* str;
  float weight = ((static_cast<uint8_t>(data[13]) << 8) | (static_cast<uint8_t>(data[14])))*0.01;
  Serial.print(F("Weight: "));
  Serial.println(weight);
  sendMQTT("paxyhome/yunmai/weight",weight);
  float resistance = ((static_cast<uint8_t>(data[15]) << 8) | (static_cast<uint8_t>(data[16])));
  Serial.print(F("Restistance: "));
  Serial.println(resistance);
  if(resistance>0)
  {
    float fat=getFat(age,weight,resistance);
    Serial.print(F("Fat: "));
    Serial.println(fat);
    sendMQTT("paxyhome/yunmai/fat",fat);
    float muscle=getMuscle(fat);
    Serial.print(F("Muscle: "));
    Serial.println(muscle);
    sendMQTT("paxyhome/yunmai/muscle",muscle);
    float water=getWater(fat);
    Serial.print(F("Water: "));
    Serial.println(water);
    sendMQTT("paxyhome/yunmai/water",water);
    float boneMass=getBoneMass(muscle,weight);
    Serial.print(F("Bone mass: "));
    Serial.println(boneMass);
    sendMQTT("paxyhome/yunmai/boneMass",boneMass);
    float skeletalMuscle=getSkeletalMuscle(fat);
    Serial.print(F("Skeletal muscle: "));
    Serial.println(skeletalMuscle);
    sendMQTT("paxyhome/yunmai/skeletalMuscle",skeletalMuscle);
    float leanBodyMass=getLeanBodyMass(weight,fat);
    Serial.print(F("Lean body mass: "));
    Serial.println(leanBodyMass);
    sendMQTT("paxyhome/yunmai/leanBodyMass",leanBodyMass);
    float visceralFat=getVisceralFat(fat,age);
    Serial.print(F("Visceral fat: "));
    Serial.println(visceralFat);
    sendMQTT("paxyhome/yunmai/visceralFat",visceralFat);
  }
}


//Yunailib
float getWater(float bodyFat) {
  return ((100.0f - bodyFat) * 0.726f * 100.0f + 0.5f) / 100.0f;
}

float getFat(int age, float weight, int resistance) {
  // for < 0x1e version devices
  float fat;

  float r = (resistance - 100.0f) / 100.0f;
  float h = height / 100.0f;

  if (r >= 1) {
      r = sqrt(r);
  }

  fat = (weight * 1.5f / h / h) + (age * 0.08f);
  if (sex == 1) {
      fat -= 10.8f;
  }

  fat = (fat - 7.4f) + r;

  if (fat < 5.0f || fat > 75.0f) {
      fat = 0.0f;
  }

  return fat;
}

float getMuscle(float bodyFat) {
  float muscle;
  muscle = (100.0f - bodyFat) * 0.67f;

  if (fitnessBodyType) {
      muscle = (100.0f - bodyFat) * 0.7f;
  }

  muscle = ((muscle * 100.0f) + 0.5f) / 100.0f;

  return muscle;
}

float getSkeletalMuscle(float bodyFat) {
  float muscle;

  muscle = (100.0f - bodyFat) * 0.53f;
  if (fitnessBodyType) {
      muscle = (100.0f - bodyFat) * 0.6f;
  }

  muscle = ((muscle * 100.0f) + 0.5f) / 100.0f;

  return muscle;
}


float getBoneMass(float muscle, float weight) {
  float boneMass;

  float h = height - 170.0f;

  if (sex == 1) {
      boneMass = ((weight * (muscle  / 100.0f) * 4.0f) / 7.0f * 0.22f * 0.6f) + (h / 100.0f);
  } else {
      boneMass = ((weight * (muscle  / 100.0f) * 4.0f) / 7.0f * 0.34f * 0.45f) + (h / 100.0f);
  }

  boneMass = ((boneMass * 10.0f) + 0.5f) / 10.0f;

  return boneMass;
}

float getLeanBodyMass(float weight, float bodyFat) {
  return weight * (100.0f - bodyFat) / 100.0f;
}

float getVisceralFat(float bodyFat, int age) {
  float f = bodyFat;
  int a = (age < 18 || age > 120) ? 18 : age;

  float vf;
  if (!fitnessBodyType) {
      if (sex == 1) {
          if (a < 40) {
              f -= 21.0f;
          } else if (a < 60) {
              f -= 22.0f;
          } else {
              f -= 24.0f;
          }
      } else {
          if (a < 40) {
              f -= 34.0f;
          } else if (a < 60) {
              f -= 35.0f;
          } else {
              f -= 36.0f;
          }
      }

      float d = sex == 1 ? 1.4f : 1.8f;
      if (f > 0.0f) {
          d = 1.1f;
      }

      vf = (f / d) + 9.5f;
      if (vf < 1.0f) {
          return 1.0f;
      }
      if (vf > 30.0f) {
          return 30.0f;
      }
      return vf;
  } else {
      if (bodyFat > 15.0f) {
          vf = (bodyFat - 15.0f) / 1.1f + 12.0f;
      } else {
          vf = -1 * (15.0f - bodyFat) / 1.4f + 12.0f;
      }
      if (vf < 1.0f) {
          return 1.0f;
      }
      if (vf > 9.0f) {
          return 9.0f;
      }
      return vf;
  }
}
