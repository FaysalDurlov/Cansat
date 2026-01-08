#include <Wire.h>                                                                                       
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
NEW SKETCH

#include <Adafruit_ADXL345_U.h>

// Sensor objects
Adafruit_BMP280 bmp;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

void setup() {
  Serial.begin(115200);
  
  // Initialize BMP280
  if (!bmp.begin(0x76)) { 
    Serial.println("BMP280 not found!");
    while (0);
  }

  // Initialize ADXL345
  if (!accel.begin()) {
    Serial.println("ADXL345 not found!");
    while (0);
  }

  // Initialize LoRa
  LoRa.setPins(10, 9, 2); // NSS, RESET, DIO0
  if (!LoRa.begin(433E6)) { // 433 MHz
    Serial.println("Starting LoRa failed!");
    while (0);
  }

  Serial.println("LoRa Transmitter Ready");
}

void loop() {
  // BMP280 readings
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F; // hPa
  float altitude = bmp.readAltitude(1013.25);    // Standard sea level pressure
  
  // ADXL345 readings
  sensors_event_t event;
  accel.getEvent(&event);
  float accX = event.acceleration.x;
  float accY = event.acceleration.y;
  float accZ = event.acceleration.z;

  // Create telemetry string
  String data = "";
  data += "T:" + String(temperature, 2) + ",";
  data += "P:" + String(pressure, 2) + ",";
  data += "A:" + String(altitude, 2) + ",";
  data += "X:" + String(accX, 2) + ",";
  data += "Y:" + String(accY, 2) + ",";
  data += "Z:" + String(accZ, 2);

  // Send LoRa packet
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  Serial.println("Sent: " + data);

  delay(1000); // 1000ms delay
}
