/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
String resStr;
String chipId;
String readString;

int pb = 4; // pushbutton
int rled1 = 12;
int rled2 = 13;
int rled3 = 14;
int rled4 = 15;
int rled5 = 16;

int gled1 = 17;
int gled2 = 18;
int gled3 = 19;
int gled4 = 21;
int gled5 = 22;

int bled1 = 23;
int bled2 = 25;
int bled3 = 26;
int bled4 = 27;
int bled5 = 32;

int pbCounter = 0;

// String chipId;
//  See the following for generating UUIDs:
//  https://www.uuidgenerator.net/

#define SERVICE_UUID "2E209DD1-18CD-C223-8403-A2A0C1AD89CA" // UART service UUID
#define CHARACTERISTIC_UUID_RX "2E209DD2-18CD-C223-8403-A2A0C1AD89CA"
#define CHARACTERISTIC_UUID_TX "2E209DD3-18CD-C223-8403-A2A0C1AD89CA"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    oldDeviceConnected = deviceConnected;
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("蓝牙断开");
    deviceConnected = false;
    if (!deviceConnected && oldDeviceConnected)
    {
      delay(500);                  // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
    }
  }
};
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
        resStr += rxValue[i];
      }
      Serial.println();
      if (resStr == "getid")
      {
        //Serial.println("发送ID");
        //pTxCharacteristic->setValue(chipId.c_str());
        //pTxCharacteristic->notify();

      }
      else if (resStr == "get_pbCounter")
      {
        String pbCounter_text = "pbCounter=" + String(pbCounter);
        pTxCharacteristic->setValue(pbCounter_text.c_str());
        pTxCharacteristic->notify();
      }
      else if (resStr == "power_on")
      {
        pbCounter = 1;
        
        Serial.println("当前状态：电源开，切换至照明模式");
      }
      else if (resStr == "power_off")
      {
        pbCounter = 0;
        
        Serial.println("当前状态：电源关，切换至待机模式");
      }
      else if (resStr == "mode_lit")
      {
        pbCounter = 1;
        
        Serial.println("当前状态：切换至照明模式");
      }
      else if (resStr == "mode_atm")
      {
        pbCounter = 2;
        
        Serial.println("当前状态：切换至氛围模式");
      }
      resStr = "";
    }
  }
};

void ModeSwitch()
{
  detachInterrupt(pb);
  // while(digitalRead(pb) == HIGH);
  Serial.println("OnePress");
  pbCounter++;
  pbCounter = pbCounter % 3;
  Serial.println(pbCounter);
  delay(50); //冷却时间还需商榷
  attachInterrupt(pb, ModeSwitch, RISING);
}

void setup()
{
  Serial.begin(9600);
  chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();
  //  chipid =ESP.getEfuseMac();
  //  Serial.printf("Chip id: %s\n", chipid.c_str());
  Serial.println(chipId);
  // Create the BLE Device
  BLEDevice::init("ESP32-light");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  pinMode(pb, INPUT_PULLUP);
  attachInterrupt(pb, ModeSwitch, RISING); //中断——按键监听

  for (int i = 12; i <= 27; i++)
  {
    if (i == 20 || i == 24)
      continue;
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);
}


void loop()
{
  if (pbCounter == 0)
  {
    for (int i = 12; i <= 27; i++)
    {
      if (i == 20 || i == 24)
        continue;
      digitalWrite(i, LOW);
    }
    digitalWrite(32, LOW);
    while (pbCounter == 0)
    {
      delay(10);

    }
  }
  // ModeSwitch();
  /*********************待机模式**************************/

  /*********************照明模式**************************/
  if (pbCounter == 1)
  {
    for (int i = 12; i <= 27; i++)
    {
      if (i == 20 || i == 24)
        continue;
      digitalWrite(i, HIGH);
    }
    digitalWrite(32, HIGH); //点亮全部led

    while (pbCounter == 1)
    {
      delay(10);

    }
    // ModeSwitch();              //不断扫描模式信号
  }
  /*********************照明模式**************************/

  /*********************流水模式**************************/
  if (pbCounter == 2)
  {
    // int dt=0;
    // int pastt=millis();
    for (int i = 12; i <= 27; i++)
    {
      if (i == 20 || i == 24)
        continue;
      digitalWrite(i, LOW);
    }
    digitalWrite(32, LOW);

    while (1)
    {
      for (int i = 12; i <= 27; i++)
      {
        if (i == 20 || i == 24)
          continue;
        digitalWrite(i, HIGH);
        delay(70);
        if (pbCounter != 2)
          break;
        digitalWrite(i, LOW);
      }
      if (pbCounter != 2)
        break;

      digitalWrite(32, HIGH);
      delay(70);
      if (pbCounter != 2)
        break;
      digitalWrite(32, LOW);
      delay(10);

    }

    if (pbCounter != 2)
    {
      for (int i = 12; i <= 27; i++)
      {
        if (i == 20 || i == 24)
          continue;
        digitalWrite(i, LOW);
      }
      digitalWrite(32, LOW);
    }
    delay(10);
  }

}
