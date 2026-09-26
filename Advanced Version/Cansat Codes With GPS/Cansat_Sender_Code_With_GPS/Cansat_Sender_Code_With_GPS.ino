#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include "Adafruit_BME680.h"
#include <TinyGPSPlus.h>

// ---------- Pin Definitions ----------
#define I2C_SCL   18
#define I2C_SDA   19

#define LORA_SCK  1
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_NSS  0
#define LORA_RST  7
#define LORA_DIO0 6

// GPS UART pins (ESP32 side)
#define GPS_RX_PIN 21   // ESP32 RX <- GPS TX
#define GPS_TX_PIN 20   // ESP32 TX -> GPS RX
#define GPS_BAUD   9600

// Set this to match your LoRa module/region: 433E6, 868E6 or 915E6
#define LORA_FREQUENCY 433E6

// ---------- MPU (raw register access, works for 9250/6500/9255) ----------
#define MPU_ADDR 0x68
int16_t ax, ay, az, gx, gy, gz;
int16_t mpuTempRaw;

// ---------- BME688 ----------
Adafruit_BME680 bme;

// ---------- GPS ----------
TinyGPSPlus gps;
HardwareSerial GPSSerial(1); // use UART1

unsigned long packetCount = 0;

void mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // wake the sensor up
  Wire.endTransmission(true);
}

void mpuRead() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
  mpuTempRaw = (Wire.read() << 8) | Wire.read();
  gx = (Wire.read() << 8) | Wire.read();
  gy = (Wire.read() << 8) | Wire.read();
  gz = (Wire.read() << 8) | Wire.read();
}

// Feed any bytes currently waiting from the GPS module into TinyGPS++
void gpsFeed() {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("CanSat Transmitter booting...");

  // ---- I2C init ----
  Wire.begin(I2C_SDA, I2C_SCL);

  // ---- MPU init ----
  mpuInit();

  // ---- BME688 init ----
  if (!bme.begin(0x76)) {          // try primary address
    if (!bme.begin(0x77)) {        // fallback address
      Serial.println("Could not find BME688 sensor, check wiring!");
      while (1) delay(10);
    }
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320 degC target temp for 150 ms

  // ---- GPS init ----
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // ---- LoRa init (custom SPI pins) ----
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed. Check connections/frequency.");
    while (1) delay(10);
  }

  Serial.println("LoRa init OK. Transmitter ready.");
}

void loop() {
  // ---- Read MPU ----
  mpuRead();
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  float gyroX  = gx / 131.0;
  float gyroY  = gy / 131.0;
  float gyroZ  = gz / 131.0;
  float mpuTempC = (mpuTempRaw / 340.0) + 36.53;

  // ---- Read BME688 ----
  if (!bme.performReading()) {
    Serial.println("BME688 reading failed");
    delay(500);
    return;
  }
  float bmeTemp     = bme.temperature;
  float bmeHumidity = bme.humidity;
  float bmePressure = bme.pressure / 100.0;      // hPa
  float bmeGas      = bme.gas_resistance / 1000.0; // kOhm

  // ---- Read GPS (non-blocking accumulate, then snapshot) ----
  gpsFeed();

  double gpsLat = gps.location.isValid() ? gps.location.lat() : 0.0;
  double gpsLon = gps.location.isValid() ? gps.location.lng() : 0.0;
  float  gpsAlt = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
  int    gpsSats = gps.satellites.isValid() ? gps.satellites.value() : 0;
  int    gpsFix  = gps.location.isValid() ? 1 : 0;

  // ---- Build CSV packet ----
  // id,ax,ay,az,gx,gy,gz,mpuTemp,bmeTemp,hum,pres,gas,lat,lon,alt,sats,fix
  String packet = String(packetCount) + "," +
                  String(accelX, 3) + "," + String(accelY, 3) + "," + String(accelZ, 3) + "," +
                  String(gyroX, 2) + "," + String(gyroY, 2) + "," + String(gyroZ, 2) + "," +
                  String(mpuTempC, 2) + "," +
                  String(bmeTemp, 2) + "," + String(bmeHumidity, 2) + "," +
                  String(bmePressure, 2) + "," + String(bmeGas, 2) + "," +
                  String(gpsLat, 6) + "," + String(gpsLon, 6) + "," +
                  String(gpsAlt, 2) + "," + String(gpsSats) + "," + String(gpsFix);

  // ---- Send over LoRa ----
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.print("Sent: ");
  Serial.println(packet);

  packetCount++;
  delay(1000); // send once per second — adjust as needed
}