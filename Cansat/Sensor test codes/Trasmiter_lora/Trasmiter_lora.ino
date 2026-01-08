#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADXL345_U.h>

// Pin configuration for LoRa
#define SS   10
#define RST  9
#define DIO0 2

Adafruit_BMP280 bmp;       
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // --- Init BMP280 ---
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find BMP280!");
    while (1);
  }

  // --- Init ADXL345 ---
  if (!accel.begin()) {
    Serial.println("No ADXL345 detected!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  // --- Init LoRa ---
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {  // Bangladesh frequency = 433 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Transmitter ready!");
}

void loop() {
  // Read BMP280
  float temperature = bmp.readTemperature();
  float pressure    = bmp.readPressure() / 100.0F;
  float altitude    = bmp.readAltitude(1013.25);

  // Read ADXL345
  sensors_event_t event;
  accel.getEvent(&event);

  // Build message
  String message = "Temp:" + String(temperature) + "C, " +
                   "Pressure:" + String(pressure) + "hPa, " +
                   "Alt:" + String(altitude) + "m, " +
                   "X:" + String(event.acceleration.x) + 
                   " Y:" + String(event.acceleration.y) + 
                   " Z:" + String(event.acceleration.z);

  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.println("Sent: " + message);
  delay(2000); // send every 2 sec
}

