#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include "Adafruit_BME680.h"

// ---------- Pin Definitions ----------
#define I2C_SCL   18
#define I2C_SDA   19

#define LORA_SCK  1
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_NSS  0
#define LORA_RST  7
#define LORA_DIO0 6

// Set this to match your LoRa module/region: 433E6, 868E6 or 915E6
#define LORA_FREQUENCY 433E6

// ---------- MPU (raw register access, works for 9250/6500/9255) ----------
#define MPU_ADDR 0x68
int16_t ax, ay, az, gx, gy, gz;
int16_t mpuTempRaw;

// ---------- BME688 ----------
Adafruit_BME680 bme;

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

  // ---- Build CSV packet ----
  // id,ax,ay,az,gx,gy,gz,mpuTemp,bmeTemp,hum,pres,gas
  String packet = String(packetCount) + "," +
                  String(accelX, 3) + "," + String(accelY, 3) + "," + String(accelZ, 3) + "," +
                  String(gyroX, 2) + "," + String(gyroY, 2) + "," + String(gyroZ, 2) + "," +
                  String(mpuTempC, 2) + "," +
                  String(bmeTemp, 2) + "," + String(bmeHumidity, 2) + "," +
                  String(bmePressure, 2) + "," + String(bmeGas, 2);

  // ---- Send over LoRa ----
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.print("Sent: ");
  Serial.println(packet);

  packetCount++;
  delay(1000); // send once per second — adjust as needed
}
