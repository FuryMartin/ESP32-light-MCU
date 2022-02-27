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

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

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
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};
String resStr;
String chipId;
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
        resStr += rxValue[i];
      }
      Serial.println();
      Serial.println("*********");
      if (resStr == "getid")
      {
        pTxCharacteristic->setValue(chipId.c_str());
        pTxCharacteristic->notify();
      }
      else if (resStr == "light1on")
      {
        Serial.println("Activate: light 1");
      }
      else if (resStr == "light1off")
      {
        Serial.println("Deactivate: light 1");
      }
      else if (resStr == "light2on")
      {
        Serial.println("Activate: light 2");
      }
      else if (resStr == "light2off")
      {
        Serial.println("Deactivate: light 2");
      }
      resStr = "";
    }
  }
};

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
}
String readString;
void loop()
{

  if (deviceConnected)
  {
    //        pTxCharacteristic->setValue(&txValue, 1);
    //        pTxCharacteristic->notify();
    //        txValue++;
    //    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }
  while (Serial.available() > 0)
  {
    if (deviceConnected)
    {
      delay(3);
      readString += Serial.read();
      pTxCharacteristic->setValue(chipId.c_str());
      //      pTxCharacteristic->setValue((uint32_t)ESP.getEfuseMac());
      pTxCharacteristic->notify();
      Serial.println(chipId);
    }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
