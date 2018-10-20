#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <FreeRTOS.h>
#include <iostream>
#include <sstream>
#include <string>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#include <esp_log.h>
constexpr static const char *tag = "app_main";

extern "C" void app_main() {
  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_WRITE |
                               BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  BLEDescriptor *UserDescription = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  UserDescription->setValue("Hoge");
  pCharacteristic->addDescriptor(UserDescription);

  pCharacteristic->setValue("Hello World says Neil");
  class MyBLECharacteristicCallback : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) override {
      ESP_LOGI(tag, "onRead!");
    }
    void onWrite(BLECharacteristic *pCharacteristic) override {
      ESP_LOGI(tag, "onWrite");
    }
  };
  pCharacteristic->setCallbacks(new MyBLECharacteristicCallback());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  ESP_LOGI(tag, "Characteristic defined! Now you can read it in your phone!");

  int cnt = 0;
  while (1) {
    auto str = pCharacteristic->getValue();
    std::cout << str << std::endl;
    std::stringstream ss;
    ss << "Counter: " << cnt;
    pCharacteristic->setValue(ss.str());
    pCharacteristic->notify();

    cnt++;
    FreeRTOS::sleep(1000);
  }
}
