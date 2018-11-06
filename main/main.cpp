#include <iostream>

#include <PowerManagement.h>

#include <BLE2902.h>
#include <BLE2904.h>
#include <BLEClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

#include <esp_log.h>
constexpr static const char *tag = "app_main";

#include <FreeRTOS.h>
#include <GPIO.h>
using namespace ESP32CPP;

static BLEUUID GATT_HID((uint16_t)0x1812);
static BLEUUID GATT_HID_REPORT((uint16_t)0x2a4d);

static BLEAddress *pServerAddress = NULL;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    ESP_LOGI(tag, "onResult: %s", advertisedDevice.getName().c_str());
    if (advertisedDevice.getName().find("AB Shutter3") == 0) {
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      ESP_LOGI(tag, "found device: %s", pServerAddress->toString().c_str());
    }
  }
};

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                           uint8_t *pData, size_t length, bool isNotify) {
  uint16_t value = pData[0] << 8 | pData[1];
  ESP_LOGI(tag, "0x%04X", value);
}

extern "C" void app_main() {
  PowerManagement::init();
  gpio_num_t pin = GPIO_NUM_27;
  GPIO::setOutput(pin);
  GPIO::write(pin, 1);

  BLEDevice::init("MyESP32");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);

  while (1) {
    FreeRTOS::sleep(10);
    static bool connected = false;

    if (pServerAddress != NULL && !connected) {
      BLEClient *pClient = BLEDevice::createClient();
      pClient->connect(*pServerAddress);

      BLERemoteService *pRemoteService = pClient->getService(GATT_HID);
      if (pRemoteService) {
        auto *pCharacteristicMap = pRemoteService->getCharacteristics();
        for (auto itr : *pCharacteristicMap) {
          ESP_LOGI(tag, "%s", itr.second->toString().c_str());
          if (GATT_HID_REPORT.equals(itr.second->getUUID()) &&
              itr.second->canNotify()) {
            itr.second->registerForNotify(notifyCallback);
            ESP_LOGI(tag, "registered");
          }
        }
        connected = true;
      }
    }
  }
}
