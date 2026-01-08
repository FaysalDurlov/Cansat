#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADXL345_U.h>

// Create sensor objects
Adafruit_BMP280 bmp;       // BMP280
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); // ADXL345

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize BMP280
  if (!bmp.begin(0x76)) {   // Try 0x77 if 0x76 doesn’t work
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Initialize ADXL345
  if (!accel.begin()) {
    Serial.println("No ADXL345 detected, check wiring!");
    while (1);
  }

  accel.setRange(ADXL345_RANGE_16_G);
  Serial.println("BMP280 + ADXL345 Data Logger");
}

void loop() {
  // --- BMP280 Data ---
  float temperature = bmp.readTemperature();
  float pressure    = bmp.readPressure() / 100.0F;  // hPa
  float altitude    = bmp.readAltitude(1013.25);    // change 1013.25 to your local sea level pressure

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" °C  Pressure: ");
  Serial.print(pressure);
  Serial.print(" hPa  Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  // --- ADXL345 Data ---
  sensors_event_t event;
  accel.getEvent(&event);

  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print(" m/s^2  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print(" m/s^2  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.println(" m/s^2");

  Serial.println("-----------------------------");
  delay(1000); // update every second
}

