#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

// Pins
#define POT_PIN 34
#define BUTTON1_PIN 25
#define BUTTON2_PIN 26

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("Client disconnected");
    // Restart advertising
    pServer->getAdvertising()->start();
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Classic Wheel");

  // Pins
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // BLE setup
  BLEDevice::init("RedRocket Wheel");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Advertising started");
}

void loop() {
  if (!deviceConnected) {
    delay(500);
    return;
  }

  // Read pot
  uint16_t potValue = analogRead(POT_PIN);

  // Read buttons (LOW = pressed)
  bool b1 = digitalRead(BUTTON1_PIN) == LOW;
  bool b2 = digitalRead(BUTTON2_PIN) == LOW;

  // Prepare 4-byte packet
  uint8_t packet[4];
  packet[0] = potValue & 0xFF;
  packet[1] = (potValue >> 8) & 0xFF;
  packet[2] = b1 ? 1 : 0;
  packet[3] = b2 ? 1 : 0;

  // Send notification
  pCharacteristic->setValue(packet, 4);
  pCharacteristic->notify();

  Serial.printf("Sent: pot=%d B1=%d B2=%d success=%d\n", potValue, b1, b2);

  delay(100); // ~10 Hz updates
}